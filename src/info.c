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
 *      `uv run --project tools/femtowad femtowad info`.
 *
 *-----------------------------------------------------------------------------*/

#include "doomdef.h"
#include "sounds.h"
#include "m_fixed.h"
#include "p_mobj.h"
#include "p_enemy.h"
#include "p_pspr.h"
#include "info.h"

const char * const sprnames[NUMSPRITES] = {
  "TROO", "SHTG", "PUNG", "PISG", "PISF", "SHTF", "CHGG", "CHGF", "MISG", "MISF", "SAWG", "BLUD",
  "PUFF", "BAL1", "MISL", "TFOG", "PLAY", "POSS", "SPOS", "SARG", "BAL7", "BOSS", "ARM1", "ARM2",
  "BAR1", "BEXP", "BON1", "BON2", "BKEY", "RKEY", "YKEY", "STIM", "MEDI", "SOUL", "PINS", "SUIT",
  "PMAP", "PVIS", "CLIP", "AMMO", "ROCK", "BROK", "SHEL", "SBOX", "BPAK", "MGUN", "CSAW", "LAUN",
  "SHOT", "COLU", "POL5", "CAND", "CBRA", "ELEC", "TRED",
};

const actionf_t stateactions[NUMACTIONS] = {
  NULL,
  A_Light0,
  A_WeaponReady,
  A_Lower,
  A_Raise,
  A_Punch,
  A_ReFire,
  A_FirePistol,
  A_Light1,
  A_FireShotgun,
  A_Light2,
  A_FireCGun,
  A_GunFlash,
  A_FireMissile,
  A_Saw,
  A_Explode,
  A_Pain,
  A_PlayerScream,
  A_Fall,
  A_XScream,
  A_Look,
  A_Chase,
  A_FaceTarget,
  A_PosAttack,
  A_Scream,
  A_SPosAttack,
  A_TroopAttack,
  A_SargAttack,
  A_BruisAttack,
  A_BossDeath,
};

const state_t states[NUMSTATES] = {
  {SPR_TROO, 0, -1, AC_NULL, S_NULL},  // S_NULL
  {SPR_SHTG, 4, 0, AC_Light0, S_NULL},  // S_LIGHTDONE
  {SPR_PUNG, 0, 1, AC_WeaponReady, S_PUNCH},  // S_PUNCH
  {SPR_PUNG, 0, 1, AC_Lower, S_PUNCHDOWN},  // S_PUNCHDOWN
  {SPR_PUNG, 0, 1, AC_Raise, S_PUNCHUP},  // S_PUNCHUP
  {SPR_PUNG, 1, 4, AC_NULL, S_PUNCH2},  // S_PUNCH1
  {SPR_PUNG, 2, 4, AC_Punch, S_PUNCH3},  // S_PUNCH2
  {SPR_PUNG, 3, 5, AC_NULL, S_PUNCH4},  // S_PUNCH3
  {SPR_PUNG, 2, 4, AC_NULL, S_PUNCH5},  // S_PUNCH4
  {SPR_PUNG, 1, 5, AC_ReFire, S_PUNCH},  // S_PUNCH5
  {SPR_PISG, 0, 1, AC_WeaponReady, S_PISTOL},  // S_PISTOL
  {SPR_PISG, 0, 1, AC_Lower, S_PISTOLDOWN},  // S_PISTOLDOWN
  {SPR_PISG, 0, 1, AC_Raise, S_PISTOLUP},  // S_PISTOLUP
  {SPR_PISG, 0, 4, AC_NULL, S_PISTOL2},  // S_PISTOL1
  {SPR_PISG, 1, 6, AC_FirePistol, S_PISTOL3},  // S_PISTOL2
  {SPR_PISG, 2, 4, AC_NULL, S_PISTOL4},  // S_PISTOL3
  {SPR_PISG, 1, 5, AC_ReFire, S_PISTOL},  // S_PISTOL4
  {SPR_PISF, FF_FULLBRIGHT|0, 7, AC_Light1, S_LIGHTDONE},  // S_PISTOLFLASH
  {SPR_SHTG, 0, 1, AC_WeaponReady, S_SGUN},  // S_SGUN
  {SPR_SHTG, 0, 1, AC_Lower, S_SGUNDOWN},  // S_SGUNDOWN
  {SPR_SHTG, 0, 1, AC_Raise, S_SGUNUP},  // S_SGUNUP
  {SPR_SHTG, 0, 3, AC_NULL, S_SGUN2},  // S_SGUN1
  {SPR_SHTG, 0, 7, AC_FireShotgun, S_SGUN3},  // S_SGUN2
  {SPR_SHTG, 1, 5, AC_NULL, S_SGUN4},  // S_SGUN3
  {SPR_SHTG, 2, 5, AC_NULL, S_SGUN5},  // S_SGUN4
  {SPR_SHTG, 3, 4, AC_NULL, S_SGUN6},  // S_SGUN5
  {SPR_SHTG, 2, 5, AC_NULL, S_SGUN7},  // S_SGUN6
  {SPR_SHTG, 1, 5, AC_NULL, S_SGUN8},  // S_SGUN7
  {SPR_SHTG, 0, 3, AC_NULL, S_SGUN9},  // S_SGUN8
  {SPR_SHTG, 0, 7, AC_ReFire, S_SGUN},  // S_SGUN9
  {SPR_SHTF, FF_FULLBRIGHT|0, 4, AC_Light1, S_SGUNFLASH2},  // S_SGUNFLASH1
  {SPR_SHTF, FF_FULLBRIGHT|1, 3, AC_Light2, S_LIGHTDONE},  // S_SGUNFLASH2
  {SPR_CHGG, 0, 1, AC_WeaponReady, S_CHAIN},  // S_CHAIN
  {SPR_CHGG, 0, 1, AC_Lower, S_CHAINDOWN},  // S_CHAINDOWN
  {SPR_CHGG, 0, 1, AC_Raise, S_CHAINUP},  // S_CHAINUP
  {SPR_CHGG, 0, 4, AC_FireCGun, S_CHAIN2},  // S_CHAIN1
  {SPR_CHGG, 1, 4, AC_FireCGun, S_CHAIN3},  // S_CHAIN2
  {SPR_CHGG, 1, 0, AC_ReFire, S_CHAIN},  // S_CHAIN3
  {SPR_CHGF, FF_FULLBRIGHT|0, 5, AC_Light1, S_LIGHTDONE},  // S_CHAINFLASH1
  {SPR_CHGF, FF_FULLBRIGHT|1, 5, AC_Light2, S_LIGHTDONE},  // S_CHAINFLASH2
  {SPR_MISG, 0, 1, AC_WeaponReady, S_MISSILE},  // S_MISSILE
  {SPR_MISG, 0, 1, AC_Lower, S_MISSILEDOWN},  // S_MISSILEDOWN
  {SPR_MISG, 0, 1, AC_Raise, S_MISSILEUP},  // S_MISSILEUP
  {SPR_MISG, 1, 8, AC_GunFlash, S_MISSILE2},  // S_MISSILE1
  {SPR_MISG, 1, 12, AC_FireMissile, S_MISSILE3},  // S_MISSILE2
  {SPR_MISG, 1, 0, AC_ReFire, S_MISSILE},  // S_MISSILE3
  {SPR_MISF, FF_FULLBRIGHT|0, 3, AC_Light1, S_MISSILEFLASH2},  // S_MISSILEFLASH1
  {SPR_MISF, FF_FULLBRIGHT|1, 4, AC_NULL, S_MISSILEFLASH3},  // S_MISSILEFLASH2
  {SPR_MISF, FF_FULLBRIGHT|2, 4, AC_Light2, S_MISSILEFLASH4},  // S_MISSILEFLASH3
  {SPR_MISF, FF_FULLBRIGHT|3, 4, AC_Light2, S_LIGHTDONE},  // S_MISSILEFLASH4
  {SPR_SAWG, 2, 4, AC_WeaponReady, S_SAWB},  // S_SAW
  {SPR_SAWG, 3, 4, AC_WeaponReady, S_SAW},  // S_SAWB
  {SPR_SAWG, 2, 1, AC_Lower, S_SAWDOWN},  // S_SAWDOWN
  {SPR_SAWG, 2, 1, AC_Raise, S_SAWUP},  // S_SAWUP
  {SPR_SAWG, 0, 4, AC_Saw, S_SAW2},  // S_SAW1
  {SPR_SAWG, 1, 4, AC_Saw, S_SAW3},  // S_SAW2
  {SPR_SAWG, 1, 0, AC_ReFire, S_SAW},  // S_SAW3
  {SPR_BLUD, 2, 8, AC_NULL, S_BLOOD2},  // S_BLOOD1
  {SPR_BLUD, 1, 8, AC_NULL, S_BLOOD3},  // S_BLOOD2
  {SPR_BLUD, 0, 8, AC_NULL, S_NULL},  // S_BLOOD3
  {SPR_PUFF, FF_FULLBRIGHT|0, 4, AC_NULL, S_PUFF2},  // S_PUFF1
  {SPR_PUFF, 1, 4, AC_NULL, S_PUFF3},  // S_PUFF2
  {SPR_PUFF, 2, 4, AC_NULL, S_PUFF4},  // S_PUFF3
  {SPR_PUFF, 3, 4, AC_NULL, S_NULL},  // S_PUFF4
  {SPR_BAL1, FF_FULLBRIGHT|0, 4, AC_NULL, S_TBALL2},  // S_TBALL1
  {SPR_BAL1, FF_FULLBRIGHT|1, 4, AC_NULL, S_TBALL1},  // S_TBALL2
  {SPR_BAL1, FF_FULLBRIGHT|2, 6, AC_NULL, S_TBALLX2},  // S_TBALLX1
  {SPR_BAL1, FF_FULLBRIGHT|3, 6, AC_NULL, S_TBALLX3},  // S_TBALLX2
  {SPR_BAL1, FF_FULLBRIGHT|4, 6, AC_NULL, S_NULL},  // S_TBALLX3
  {SPR_MISL, FF_FULLBRIGHT|0, 1, AC_NULL, S_ROCKET},  // S_ROCKET
  {SPR_MISL, FF_FULLBRIGHT|1, 8, AC_Explode, S_EXPLODE2},  // S_EXPLODE1
  {SPR_MISL, FF_FULLBRIGHT|2, 6, AC_NULL, S_EXPLODE3},  // S_EXPLODE2
  {SPR_MISL, FF_FULLBRIGHT|3, 4, AC_NULL, S_NULL},  // S_EXPLODE3
  {SPR_TFOG, FF_FULLBRIGHT|0, 6, AC_NULL, S_TFOG01},  // S_TFOG
  {SPR_TFOG, FF_FULLBRIGHT|1, 6, AC_NULL, S_TFOG02},  // S_TFOG01
  {SPR_TFOG, FF_FULLBRIGHT|0, 6, AC_NULL, S_TFOG2},  // S_TFOG02
  {SPR_TFOG, FF_FULLBRIGHT|1, 6, AC_NULL, S_TFOG3},  // S_TFOG2
  {SPR_TFOG, FF_FULLBRIGHT|2, 6, AC_NULL, S_TFOG4},  // S_TFOG3
  {SPR_TFOG, FF_FULLBRIGHT|3, 6, AC_NULL, S_TFOG5},  // S_TFOG4
  {SPR_TFOG, FF_FULLBRIGHT|4, 6, AC_NULL, S_TFOG6},  // S_TFOG5
  {SPR_TFOG, FF_FULLBRIGHT|5, 6, AC_NULL, S_TFOG7},  // S_TFOG6
  {SPR_TFOG, FF_FULLBRIGHT|6, 6, AC_NULL, S_TFOG8},  // S_TFOG7
  {SPR_TFOG, FF_FULLBRIGHT|7, 6, AC_NULL, S_TFOG9},  // S_TFOG8
  {SPR_TFOG, FF_FULLBRIGHT|8, 6, AC_NULL, S_TFOG10},  // S_TFOG9
  {SPR_TFOG, FF_FULLBRIGHT|9, 6, AC_NULL, S_NULL},  // S_TFOG10
  {SPR_PLAY, 0, -1, AC_NULL, S_NULL},  // S_PLAY
  {SPR_PLAY, 0, 4, AC_NULL, S_PLAY_RUN2},  // S_PLAY_RUN1
  {SPR_PLAY, 1, 4, AC_NULL, S_PLAY_RUN3},  // S_PLAY_RUN2
  {SPR_PLAY, 2, 4, AC_NULL, S_PLAY_RUN4},  // S_PLAY_RUN3
  {SPR_PLAY, 3, 4, AC_NULL, S_PLAY_RUN1},  // S_PLAY_RUN4
  {SPR_PLAY, 4, 12, AC_NULL, S_PLAY},  // S_PLAY_ATK1
  {SPR_PLAY, FF_FULLBRIGHT|5, 6, AC_NULL, S_PLAY_ATK1},  // S_PLAY_ATK2
  {SPR_PLAY, 6, 4, AC_NULL, S_PLAY_PAIN2},  // S_PLAY_PAIN
  {SPR_PLAY, 6, 4, AC_Pain, S_PLAY},  // S_PLAY_PAIN2
  {SPR_PLAY, 7, 10, AC_NULL, S_PLAY_DIE2},  // S_PLAY_DIE1
  {SPR_PLAY, 8, 10, AC_PlayerScream, S_PLAY_DIE3},  // S_PLAY_DIE2
  {SPR_PLAY, 9, 10, AC_Fall, S_PLAY_DIE4},  // S_PLAY_DIE3
  {SPR_PLAY, 10, 10, AC_NULL, S_PLAY_DIE5},  // S_PLAY_DIE4
  {SPR_PLAY, 11, 10, AC_NULL, S_PLAY_DIE6},  // S_PLAY_DIE5
  {SPR_PLAY, 12, 10, AC_NULL, S_PLAY_DIE7},  // S_PLAY_DIE6
  {SPR_PLAY, 13, -1, AC_NULL, S_NULL},  // S_PLAY_DIE7
  {SPR_PLAY, 14, 5, AC_NULL, S_PLAY_XDIE2},  // S_PLAY_XDIE1
  {SPR_PLAY, 15, 5, AC_XScream, S_PLAY_XDIE3},  // S_PLAY_XDIE2
  {SPR_PLAY, 16, 5, AC_Fall, S_PLAY_XDIE4},  // S_PLAY_XDIE3
  {SPR_PLAY, 17, 5, AC_NULL, S_PLAY_XDIE5},  // S_PLAY_XDIE4
  {SPR_PLAY, 18, 5, AC_NULL, S_PLAY_XDIE6},  // S_PLAY_XDIE5
  {SPR_PLAY, 19, 5, AC_NULL, S_PLAY_XDIE7},  // S_PLAY_XDIE6
  {SPR_PLAY, 20, 5, AC_NULL, S_PLAY_XDIE8},  // S_PLAY_XDIE7
  {SPR_PLAY, 21, 5, AC_NULL, S_PLAY_XDIE9},  // S_PLAY_XDIE8
  {SPR_PLAY, 22, -1, AC_NULL, S_NULL},  // S_PLAY_XDIE9
  {SPR_POSS, 0, 10, AC_Look, S_POSS_STND2},  // S_POSS_STND
  {SPR_POSS, 1, 10, AC_Look, S_POSS_STND},  // S_POSS_STND2
  {SPR_POSS, 0, 4, AC_Chase, S_POSS_RUN2},  // S_POSS_RUN1
  {SPR_POSS, 0, 4, AC_Chase, S_POSS_RUN3},  // S_POSS_RUN2
  {SPR_POSS, 1, 4, AC_Chase, S_POSS_RUN4},  // S_POSS_RUN3
  {SPR_POSS, 1, 4, AC_Chase, S_POSS_RUN5},  // S_POSS_RUN4
  {SPR_POSS, 2, 4, AC_Chase, S_POSS_RUN6},  // S_POSS_RUN5
  {SPR_POSS, 2, 4, AC_Chase, S_POSS_RUN7},  // S_POSS_RUN6
  {SPR_POSS, 3, 4, AC_Chase, S_POSS_RUN8},  // S_POSS_RUN7
  {SPR_POSS, 3, 4, AC_Chase, S_POSS_RUN1},  // S_POSS_RUN8
  {SPR_POSS, 4, 10, AC_FaceTarget, S_POSS_ATK2},  // S_POSS_ATK1
  {SPR_POSS, 5, 8, AC_PosAttack, S_POSS_ATK3},  // S_POSS_ATK2
  {SPR_POSS, 4, 8, AC_NULL, S_POSS_RUN1},  // S_POSS_ATK3
  {SPR_POSS, 6, 3, AC_NULL, S_POSS_PAIN2},  // S_POSS_PAIN
  {SPR_POSS, 6, 3, AC_Pain, S_POSS_RUN1},  // S_POSS_PAIN2
  {SPR_POSS, 7, 5, AC_NULL, S_POSS_DIE2},  // S_POSS_DIE1
  {SPR_POSS, 8, 5, AC_Scream, S_POSS_DIE3},  // S_POSS_DIE2
  {SPR_POSS, 9, 5, AC_Fall, S_POSS_DIE4},  // S_POSS_DIE3
  {SPR_POSS, 10, 5, AC_NULL, S_POSS_DIE5},  // S_POSS_DIE4
  {SPR_POSS, 11, -1, AC_NULL, S_NULL},  // S_POSS_DIE5
  {SPR_POSS, 12, 5, AC_NULL, S_POSS_XDIE2},  // S_POSS_XDIE1
  {SPR_POSS, 13, 5, AC_XScream, S_POSS_XDIE3},  // S_POSS_XDIE2
  {SPR_POSS, 14, 5, AC_Fall, S_POSS_XDIE4},  // S_POSS_XDIE3
  {SPR_POSS, 15, 5, AC_NULL, S_POSS_XDIE5},  // S_POSS_XDIE4
  {SPR_POSS, 16, 5, AC_NULL, S_POSS_XDIE6},  // S_POSS_XDIE5
  {SPR_POSS, 17, 5, AC_NULL, S_POSS_XDIE7},  // S_POSS_XDIE6
  {SPR_POSS, 18, 5, AC_NULL, S_POSS_XDIE8},  // S_POSS_XDIE7
  {SPR_POSS, 19, 5, AC_NULL, S_POSS_XDIE9},  // S_POSS_XDIE8
  {SPR_POSS, 20, -1, AC_NULL, S_NULL},  // S_POSS_XDIE9
  {SPR_SPOS, 0, 10, AC_Look, S_SPOS_STND2},  // S_SPOS_STND
  {SPR_SPOS, 1, 10, AC_Look, S_SPOS_STND},  // S_SPOS_STND2
  {SPR_SPOS, 0, 3, AC_Chase, S_SPOS_RUN2},  // S_SPOS_RUN1
  {SPR_SPOS, 0, 3, AC_Chase, S_SPOS_RUN3},  // S_SPOS_RUN2
  {SPR_SPOS, 1, 3, AC_Chase, S_SPOS_RUN4},  // S_SPOS_RUN3
  {SPR_SPOS, 1, 3, AC_Chase, S_SPOS_RUN5},  // S_SPOS_RUN4
  {SPR_SPOS, 2, 3, AC_Chase, S_SPOS_RUN6},  // S_SPOS_RUN5
  {SPR_SPOS, 2, 3, AC_Chase, S_SPOS_RUN7},  // S_SPOS_RUN6
  {SPR_SPOS, 3, 3, AC_Chase, S_SPOS_RUN8},  // S_SPOS_RUN7
  {SPR_SPOS, 3, 3, AC_Chase, S_SPOS_RUN1},  // S_SPOS_RUN8
  {SPR_SPOS, 4, 10, AC_FaceTarget, S_SPOS_ATK2},  // S_SPOS_ATK1
  {SPR_SPOS, FF_FULLBRIGHT|5, 10, AC_SPosAttack, S_SPOS_ATK3},  // S_SPOS_ATK2
  {SPR_SPOS, 4, 10, AC_NULL, S_SPOS_RUN1},  // S_SPOS_ATK3
  {SPR_SPOS, 6, 3, AC_NULL, S_SPOS_PAIN2},  // S_SPOS_PAIN
  {SPR_SPOS, 6, 3, AC_Pain, S_SPOS_RUN1},  // S_SPOS_PAIN2
  {SPR_SPOS, 7, 5, AC_NULL, S_SPOS_DIE2},  // S_SPOS_DIE1
  {SPR_SPOS, 8, 5, AC_Scream, S_SPOS_DIE3},  // S_SPOS_DIE2
  {SPR_SPOS, 9, 5, AC_Fall, S_SPOS_DIE4},  // S_SPOS_DIE3
  {SPR_SPOS, 10, 5, AC_NULL, S_SPOS_DIE5},  // S_SPOS_DIE4
  {SPR_SPOS, 11, -1, AC_NULL, S_NULL},  // S_SPOS_DIE5
  {SPR_SPOS, 12, 5, AC_NULL, S_SPOS_XDIE2},  // S_SPOS_XDIE1
  {SPR_SPOS, 13, 5, AC_XScream, S_SPOS_XDIE3},  // S_SPOS_XDIE2
  {SPR_SPOS, 14, 5, AC_Fall, S_SPOS_XDIE4},  // S_SPOS_XDIE3
  {SPR_SPOS, 15, 5, AC_NULL, S_SPOS_XDIE5},  // S_SPOS_XDIE4
  {SPR_SPOS, 16, 5, AC_NULL, S_SPOS_XDIE6},  // S_SPOS_XDIE5
  {SPR_SPOS, 17, 5, AC_NULL, S_SPOS_XDIE7},  // S_SPOS_XDIE6
  {SPR_SPOS, 18, 5, AC_NULL, S_SPOS_XDIE8},  // S_SPOS_XDIE7
  {SPR_SPOS, 19, 5, AC_NULL, S_SPOS_XDIE9},  // S_SPOS_XDIE8
  {SPR_SPOS, 20, -1, AC_NULL, S_NULL},  // S_SPOS_XDIE9
  {SPR_TROO, 0, 10, AC_Look, S_TROO_STND2},  // S_TROO_STND
  {SPR_TROO, 1, 10, AC_Look, S_TROO_STND},  // S_TROO_STND2
  {SPR_TROO, 0, 3, AC_Chase, S_TROO_RUN2},  // S_TROO_RUN1
  {SPR_TROO, 0, 3, AC_Chase, S_TROO_RUN3},  // S_TROO_RUN2
  {SPR_TROO, 1, 3, AC_Chase, S_TROO_RUN4},  // S_TROO_RUN3
  {SPR_TROO, 1, 3, AC_Chase, S_TROO_RUN5},  // S_TROO_RUN4
  {SPR_TROO, 2, 3, AC_Chase, S_TROO_RUN6},  // S_TROO_RUN5
  {SPR_TROO, 2, 3, AC_Chase, S_TROO_RUN7},  // S_TROO_RUN6
  {SPR_TROO, 3, 3, AC_Chase, S_TROO_RUN8},  // S_TROO_RUN7
  {SPR_TROO, 3, 3, AC_Chase, S_TROO_RUN1},  // S_TROO_RUN8
  {SPR_TROO, 4, 8, AC_FaceTarget, S_TROO_ATK2},  // S_TROO_ATK1
  {SPR_TROO, 5, 8, AC_FaceTarget, S_TROO_ATK3},  // S_TROO_ATK2
  {SPR_TROO, 6, 6, AC_TroopAttack, S_TROO_RUN1},  // S_TROO_ATK3
  {SPR_TROO, 7, 2, AC_NULL, S_TROO_PAIN2},  // S_TROO_PAIN
  {SPR_TROO, 7, 2, AC_Pain, S_TROO_RUN1},  // S_TROO_PAIN2
  {SPR_TROO, 8, 8, AC_NULL, S_TROO_DIE2},  // S_TROO_DIE1
  {SPR_TROO, 9, 8, AC_Scream, S_TROO_DIE3},  // S_TROO_DIE2
  {SPR_TROO, 10, 6, AC_NULL, S_TROO_DIE4},  // S_TROO_DIE3
  {SPR_TROO, 11, 6, AC_Fall, S_TROO_DIE5},  // S_TROO_DIE4
  {SPR_TROO, 12, -1, AC_NULL, S_NULL},  // S_TROO_DIE5
  {SPR_TROO, 13, 5, AC_NULL, S_TROO_XDIE2},  // S_TROO_XDIE1
  {SPR_TROO, 14, 5, AC_XScream, S_TROO_XDIE3},  // S_TROO_XDIE2
  {SPR_TROO, 15, 5, AC_NULL, S_TROO_XDIE4},  // S_TROO_XDIE3
  {SPR_TROO, 16, 5, AC_Fall, S_TROO_XDIE5},  // S_TROO_XDIE4
  {SPR_TROO, 17, 5, AC_NULL, S_TROO_XDIE6},  // S_TROO_XDIE5
  {SPR_TROO, 18, 5, AC_NULL, S_TROO_XDIE7},  // S_TROO_XDIE6
  {SPR_TROO, 19, 5, AC_NULL, S_TROO_XDIE8},  // S_TROO_XDIE7
  {SPR_TROO, 20, -1, AC_NULL, S_NULL},  // S_TROO_XDIE8
  {SPR_SARG, 0, 10, AC_Look, S_SARG_STND2},  // S_SARG_STND
  {SPR_SARG, 1, 10, AC_Look, S_SARG_STND},  // S_SARG_STND2
  {SPR_SARG, 0, 2, AC_Chase, S_SARG_RUN2},  // S_SARG_RUN1
  {SPR_SARG, 0, 2, AC_Chase, S_SARG_RUN3},  // S_SARG_RUN2
  {SPR_SARG, 1, 2, AC_Chase, S_SARG_RUN4},  // S_SARG_RUN3
  {SPR_SARG, 1, 2, AC_Chase, S_SARG_RUN5},  // S_SARG_RUN4
  {SPR_SARG, 2, 2, AC_Chase, S_SARG_RUN6},  // S_SARG_RUN5
  {SPR_SARG, 2, 2, AC_Chase, S_SARG_RUN7},  // S_SARG_RUN6
  {SPR_SARG, 3, 2, AC_Chase, S_SARG_RUN8},  // S_SARG_RUN7
  {SPR_SARG, 3, 2, AC_Chase, S_SARG_RUN1},  // S_SARG_RUN8
  {SPR_SARG, 4, 8, AC_FaceTarget, S_SARG_ATK2},  // S_SARG_ATK1
  {SPR_SARG, 5, 8, AC_FaceTarget, S_SARG_ATK3},  // S_SARG_ATK2
  {SPR_SARG, 6, 8, AC_SargAttack, S_SARG_RUN1},  // S_SARG_ATK3
  {SPR_SARG, 7, 2, AC_NULL, S_SARG_PAIN2},  // S_SARG_PAIN
  {SPR_SARG, 7, 2, AC_Pain, S_SARG_RUN1},  // S_SARG_PAIN2
  {SPR_SARG, 8, 8, AC_NULL, S_SARG_DIE2},  // S_SARG_DIE1
  {SPR_SARG, 9, 8, AC_Scream, S_SARG_DIE3},  // S_SARG_DIE2
  {SPR_SARG, 10, 4, AC_NULL, S_SARG_DIE4},  // S_SARG_DIE3
  {SPR_SARG, 11, 4, AC_Fall, S_SARG_DIE5},  // S_SARG_DIE4
  {SPR_SARG, 12, 4, AC_NULL, S_SARG_DIE6},  // S_SARG_DIE5
  {SPR_SARG, 13, -1, AC_NULL, S_NULL},  // S_SARG_DIE6
  {SPR_BAL7, FF_FULLBRIGHT|0, 4, AC_NULL, S_BRBALL2},  // S_BRBALL1
  {SPR_BAL7, FF_FULLBRIGHT|1, 4, AC_NULL, S_BRBALL1},  // S_BRBALL2
  {SPR_BAL7, FF_FULLBRIGHT|2, 6, AC_NULL, S_BRBALLX2},  // S_BRBALLX1
  {SPR_BAL7, FF_FULLBRIGHT|3, 6, AC_NULL, S_BRBALLX3},  // S_BRBALLX2
  {SPR_BAL7, FF_FULLBRIGHT|4, 6, AC_NULL, S_NULL},  // S_BRBALLX3
  {SPR_BOSS, 0, 10, AC_Look, S_BOSS_STND2},  // S_BOSS_STND
  {SPR_BOSS, 1, 10, AC_Look, S_BOSS_STND},  // S_BOSS_STND2
  {SPR_BOSS, 0, 3, AC_Chase, S_BOSS_RUN2},  // S_BOSS_RUN1
  {SPR_BOSS, 0, 3, AC_Chase, S_BOSS_RUN3},  // S_BOSS_RUN2
  {SPR_BOSS, 1, 3, AC_Chase, S_BOSS_RUN4},  // S_BOSS_RUN3
  {SPR_BOSS, 1, 3, AC_Chase, S_BOSS_RUN5},  // S_BOSS_RUN4
  {SPR_BOSS, 2, 3, AC_Chase, S_BOSS_RUN6},  // S_BOSS_RUN5
  {SPR_BOSS, 2, 3, AC_Chase, S_BOSS_RUN7},  // S_BOSS_RUN6
  {SPR_BOSS, 3, 3, AC_Chase, S_BOSS_RUN8},  // S_BOSS_RUN7
  {SPR_BOSS, 3, 3, AC_Chase, S_BOSS_RUN1},  // S_BOSS_RUN8
  {SPR_BOSS, 4, 8, AC_FaceTarget, S_BOSS_ATK2},  // S_BOSS_ATK1
  {SPR_BOSS, 5, 8, AC_FaceTarget, S_BOSS_ATK3},  // S_BOSS_ATK2
  {SPR_BOSS, 6, 8, AC_BruisAttack, S_BOSS_RUN1},  // S_BOSS_ATK3
  {SPR_BOSS, 7, 2, AC_NULL, S_BOSS_PAIN2},  // S_BOSS_PAIN
  {SPR_BOSS, 7, 2, AC_Pain, S_BOSS_RUN1},  // S_BOSS_PAIN2
  {SPR_BOSS, 8, 8, AC_NULL, S_BOSS_DIE2},  // S_BOSS_DIE1
  {SPR_BOSS, 9, 8, AC_Scream, S_BOSS_DIE3},  // S_BOSS_DIE2
  {SPR_BOSS, 10, 8, AC_NULL, S_BOSS_DIE4},  // S_BOSS_DIE3
  {SPR_BOSS, 11, 8, AC_Fall, S_BOSS_DIE5},  // S_BOSS_DIE4
  {SPR_BOSS, 12, 8, AC_NULL, S_BOSS_DIE6},  // S_BOSS_DIE5
  {SPR_BOSS, 13, 8, AC_NULL, S_BOSS_DIE7},  // S_BOSS_DIE6
  {SPR_BOSS, 14, -1, AC_BossDeath, S_NULL},  // S_BOSS_DIE7
  {SPR_ARM1, 0, 6, AC_NULL, S_ARM1A},  // S_ARM1
  {SPR_ARM1, FF_FULLBRIGHT|1, 7, AC_NULL, S_ARM1},  // S_ARM1A
  {SPR_ARM2, 0, 6, AC_NULL, S_ARM2A},  // S_ARM2
  {SPR_ARM2, FF_FULLBRIGHT|1, 6, AC_NULL, S_ARM2},  // S_ARM2A
  {SPR_BAR1, 0, 6, AC_NULL, S_BAR2},  // S_BAR1
  {SPR_BAR1, 1, 6, AC_NULL, S_BAR1},  // S_BAR2
  {SPR_BEXP, FF_FULLBRIGHT|0, 5, AC_NULL, S_BEXP2},  // S_BEXP
  {SPR_BEXP, FF_FULLBRIGHT|1, 5, AC_Scream, S_BEXP3},  // S_BEXP2
  {SPR_BEXP, FF_FULLBRIGHT|2, 5, AC_NULL, S_BEXP4},  // S_BEXP3
  {SPR_BEXP, FF_FULLBRIGHT|3, 10, AC_Explode, S_BEXP5},  // S_BEXP4
  {SPR_BEXP, FF_FULLBRIGHT|4, 10, AC_NULL, S_NULL},  // S_BEXP5
  {SPR_BON1, 0, 6, AC_NULL, S_BON1A},  // S_BON1
  {SPR_BON1, 1, 6, AC_NULL, S_BON1B},  // S_BON1A
  {SPR_BON1, 2, 6, AC_NULL, S_BON1C},  // S_BON1B
  {SPR_BON1, 3, 6, AC_NULL, S_BON1D},  // S_BON1C
  {SPR_BON1, 2, 6, AC_NULL, S_BON1E},  // S_BON1D
  {SPR_BON1, 1, 6, AC_NULL, S_BON1},  // S_BON1E
  {SPR_BON2, 0, 6, AC_NULL, S_BON2A},  // S_BON2
  {SPR_BON2, 1, 6, AC_NULL, S_BON2B},  // S_BON2A
  {SPR_BON2, 2, 6, AC_NULL, S_BON2C},  // S_BON2B
  {SPR_BON2, 3, 6, AC_NULL, S_BON2D},  // S_BON2C
  {SPR_BON2, 2, 6, AC_NULL, S_BON2E},  // S_BON2D
  {SPR_BON2, 1, 6, AC_NULL, S_BON2},  // S_BON2E
  {SPR_BKEY, 0, 10, AC_NULL, S_BKEY2},  // S_BKEY
  {SPR_BKEY, FF_FULLBRIGHT|1, 10, AC_NULL, S_BKEY},  // S_BKEY2
  {SPR_RKEY, 0, 10, AC_NULL, S_RKEY2},  // S_RKEY
  {SPR_RKEY, FF_FULLBRIGHT|1, 10, AC_NULL, S_RKEY},  // S_RKEY2
  {SPR_YKEY, 0, 10, AC_NULL, S_YKEY2},  // S_YKEY
  {SPR_YKEY, FF_FULLBRIGHT|1, 10, AC_NULL, S_YKEY},  // S_YKEY2
  {SPR_STIM, 0, -1, AC_NULL, S_NULL},  // S_STIM
  {SPR_MEDI, 0, -1, AC_NULL, S_NULL},  // S_MEDI
  {SPR_SOUL, FF_FULLBRIGHT|0, 6, AC_NULL, S_SOUL2},  // S_SOUL
  {SPR_SOUL, FF_FULLBRIGHT|1, 6, AC_NULL, S_SOUL3},  // S_SOUL2
  {SPR_SOUL, FF_FULLBRIGHT|2, 6, AC_NULL, S_SOUL4},  // S_SOUL3
  {SPR_SOUL, FF_FULLBRIGHT|3, 6, AC_NULL, S_SOUL5},  // S_SOUL4
  {SPR_SOUL, FF_FULLBRIGHT|2, 6, AC_NULL, S_SOUL6},  // S_SOUL5
  {SPR_SOUL, FF_FULLBRIGHT|1, 6, AC_NULL, S_SOUL},  // S_SOUL6
  {SPR_PINS, FF_FULLBRIGHT|0, 6, AC_NULL, S_PINS2},  // S_PINS
  {SPR_PINS, FF_FULLBRIGHT|1, 6, AC_NULL, S_PINS3},  // S_PINS2
  {SPR_PINS, FF_FULLBRIGHT|2, 6, AC_NULL, S_PINS4},  // S_PINS3
  {SPR_PINS, FF_FULLBRIGHT|3, 6, AC_NULL, S_PINS},  // S_PINS4
  {SPR_SUIT, FF_FULLBRIGHT|0, -1, AC_NULL, S_NULL},  // S_SUIT
  {SPR_PMAP, FF_FULLBRIGHT|0, 6, AC_NULL, S_PMAP2},  // S_PMAP
  {SPR_PMAP, FF_FULLBRIGHT|1, 6, AC_NULL, S_PMAP3},  // S_PMAP2
  {SPR_PMAP, FF_FULLBRIGHT|2, 6, AC_NULL, S_PMAP4},  // S_PMAP3
  {SPR_PMAP, FF_FULLBRIGHT|3, 6, AC_NULL, S_PMAP5},  // S_PMAP4
  {SPR_PMAP, FF_FULLBRIGHT|2, 6, AC_NULL, S_PMAP6},  // S_PMAP5
  {SPR_PMAP, FF_FULLBRIGHT|1, 6, AC_NULL, S_PMAP},  // S_PMAP6
  {SPR_PVIS, FF_FULLBRIGHT|0, 6, AC_NULL, S_PVIS2},  // S_PVIS
  {SPR_PVIS, 1, 6, AC_NULL, S_PVIS},  // S_PVIS2
  {SPR_CLIP, 0, -1, AC_NULL, S_NULL},  // S_CLIP
  {SPR_AMMO, 0, -1, AC_NULL, S_NULL},  // S_AMMO
  {SPR_ROCK, 0, -1, AC_NULL, S_NULL},  // S_ROCK
  {SPR_BROK, 0, -1, AC_NULL, S_NULL},  // S_BROK
  {SPR_SHEL, 0, -1, AC_NULL, S_NULL},  // S_SHEL
  {SPR_SBOX, 0, -1, AC_NULL, S_NULL},  // S_SBOX
  {SPR_BPAK, 0, -1, AC_NULL, S_NULL},  // S_BPAK
  {SPR_MGUN, 0, -1, AC_NULL, S_NULL},  // S_MGUN
  {SPR_CSAW, 0, -1, AC_NULL, S_NULL},  // S_CSAW
  {SPR_LAUN, 0, -1, AC_NULL, S_NULL},  // S_LAUN
  {SPR_SHOT, 0, -1, AC_NULL, S_NULL},  // S_SHOT
  {SPR_COLU, FF_FULLBRIGHT|0, -1, AC_NULL, S_NULL},  // S_COLU
  {SPR_POL5, 0, -1, AC_NULL, S_NULL},  // S_GIBS
  {SPR_CAND, FF_FULLBRIGHT|0, -1, AC_NULL, S_NULL},  // S_CANDLESTIK
  {SPR_CBRA, FF_FULLBRIGHT|0, -1, AC_NULL, S_NULL},  // S_CANDELABRA
  {SPR_ELEC, 0, -1, AC_NULL, S_NULL},  // S_TECHPILLAR
  {SPR_TRED, FF_FULLBRIGHT|0, 4, AC_NULL, S_REDTORCH2},  // S_REDTORCH
  {SPR_TRED, FF_FULLBRIGHT|1, 4, AC_NULL, S_REDTORCH3},  // S_REDTORCH2
  {SPR_TRED, FF_FULLBRIGHT|2, 4, AC_NULL, S_REDTORCH4},  // S_REDTORCH3
  {SPR_TRED, FF_FULLBRIGHT|3, 4, AC_NULL, S_REDTORCH},  // S_REDTORCH4
};

const mobjinfo_t mobjinfo[NUMMOBJTYPES] = {
  {  // MT_PLAYER
    0,  // speed
    16*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP,  // flags
    -1,  // doomednum
    100,  // spawnhealth
    0,  // reactiontime
    100,  // mass
    0,  // damage
    S_PLAY,  // spawnstate
    S_PLAY_RUN1,  // seestate
    S_PLAY_PAIN,  // painstate
    S_NULL,  // meleestate
    S_PLAY_ATK1,  // missilestate
    S_PLAY_DIE1,  // deathstate
    S_PLAY_XDIE1,  // xdeathstate
    255,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_plpain,  // painsound
    sfx_pldeth,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_POSSESSED
    8,  // speed
    20*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL,  // flags
    3004,  // doomednum
    20,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_POSS_STND,  // spawnstate
    S_POSS_RUN1,  // seestate
    S_POSS_PAIN,  // painstate
    S_NULL,  // meleestate
    S_POSS_ATK1,  // missilestate
    S_POSS_DIE1,  // deathstate
    S_POSS_XDIE1,  // xdeathstate
    200,  // painchance
    sfx_posit1,  // seesound
    sfx_pistol,  // attacksound
    sfx_popain,  // painsound
    sfx_podth1,  // deathsound
    sfx_posact,  // activesound
  },
  {  // MT_SHOTGUY
    8,  // speed
    20*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL,  // flags
    9,  // doomednum
    30,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SPOS_STND,  // spawnstate
    S_SPOS_RUN1,  // seestate
    S_SPOS_PAIN,  // painstate
    S_NULL,  // meleestate
    S_SPOS_ATK1,  // missilestate
    S_SPOS_DIE1,  // deathstate
    S_SPOS_XDIE1,  // xdeathstate
    170,  // painchance
    sfx_posit2,  // seesound
    sfx_None,  // attacksound
    sfx_popain,  // painsound
    sfx_podth2,  // deathsound
    sfx_posact,  // activesound
  },
  {  // MT_TROOP
    8,  // speed
    20*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL,  // flags
    3001,  // doomednum
    60,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_TROO_STND,  // spawnstate
    S_TROO_RUN1,  // seestate
    S_TROO_PAIN,  // painstate
    S_TROO_ATK1,  // meleestate
    S_TROO_ATK1,  // missilestate
    S_TROO_DIE1,  // deathstate
    S_TROO_XDIE1,  // xdeathstate
    200,  // painchance
    sfx_bgsit1,  // seesound
    sfx_None,  // attacksound
    sfx_popain,  // painsound
    sfx_bgdth1,  // deathsound
    sfx_bgact,  // activesound
  },
  {  // MT_SERGEANT
    10,  // speed
    30*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL,  // flags
    3002,  // doomednum
    150,  // spawnhealth
    8,  // reactiontime
    400,  // mass
    0,  // damage
    S_SARG_STND,  // spawnstate
    S_SARG_RUN1,  // seestate
    S_SARG_PAIN,  // painstate
    S_SARG_ATK1,  // meleestate
    S_NULL,  // missilestate
    S_SARG_DIE1,  // deathstate
    S_NULL,  // xdeathstate
    180,  // painchance
    sfx_sgtsit,  // seesound
    sfx_sgtatk,  // attacksound
    sfx_dmpain,  // painsound
    sfx_sgtdth,  // deathsound
    sfx_dmact,  // activesound
  },
  {  // MT_SHADOWS
    10,  // speed
    30*FRACUNIT,  // radius
    56*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_SHADOW|MF_COUNTKILL,  // flags
    58,  // doomednum
    150,  // spawnhealth
    8,  // reactiontime
    400,  // mass
    0,  // damage
    S_SARG_STND,  // spawnstate
    S_SARG_RUN1,  // seestate
    S_SARG_PAIN,  // painstate
    S_SARG_ATK1,  // meleestate
    S_NULL,  // missilestate
    S_SARG_DIE1,  // deathstate
    S_NULL,  // xdeathstate
    180,  // painchance
    sfx_sgtsit,  // seesound
    sfx_sgtatk,  // attacksound
    sfx_dmpain,  // painsound
    sfx_sgtdth,  // deathsound
    sfx_dmact,  // activesound
  },
  {  // MT_BRUISER
    8,  // speed
    24*FRACUNIT,  // radius
    64*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_COUNTKILL,  // flags
    3003,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    1000,  // mass
    0,  // damage
    S_BOSS_STND,  // spawnstate
    S_BOSS_RUN1,  // seestate
    S_BOSS_PAIN,  // painstate
    S_BOSS_ATK1,  // meleestate
    S_BOSS_ATK1,  // missilestate
    S_BOSS_DIE1,  // deathstate
    S_NULL,  // xdeathstate
    50,  // painchance
    sfx_brssit,  // seesound
    sfx_None,  // attacksound
    sfx_dmpain,  // painsound
    sfx_brsdth,  // deathsound
    sfx_dmact,  // activesound
  },
  {  // MT_BRUISERSHOT
    15*FRACUNIT,  // speed
    6*FRACUNIT,  // radius
    8*FRACUNIT,  // height
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DROPOFF|MF_MISSILE,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    8,  // damage
    S_BRBALL1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_BRBALLX1,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_firsht,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_firxpl,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_BARREL
    0,  // speed
    10*FRACUNIT,  // radius
    42*FRACUNIT,  // height
    MF_SOLID|MF_SHOOTABLE|MF_NOBLOOD,  // flags
    2035,  // doomednum
    20,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BAR1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_BEXP,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_barexp,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_TROOPSHOT
    10*FRACUNIT,  // speed
    6*FRACUNIT,  // radius
    8*FRACUNIT,  // height
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DROPOFF|MF_MISSILE,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    3,  // damage
    S_TBALL1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_TBALLX1,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_firsht,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_firxpl,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_ROCKET
    20*FRACUNIT,  // speed
    11*FRACUNIT,  // radius
    8*FRACUNIT,  // height
    MF_NOBLOCKMAP|MF_NOGRAVITY|MF_DROPOFF|MF_MISSILE,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    20,  // damage
    S_ROCKET,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_EXPLODE1,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_barexp,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_PUFF
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_NOBLOCKMAP|MF_NOGRAVITY,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PUFF1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_BLOOD
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_NOBLOCKMAP,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BLOOD1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_TFOG
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_NOBLOCKMAP|MF_NOGRAVITY,  // flags
    -1,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_TFOG,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_TELEPORTMAN
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_NOSECTOR|MF_NOBLOCKMAP,  // flags
    14,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_NULL,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC0
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2018,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_ARM1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC1
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2019,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_ARM2,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC2
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2014,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BON1,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC3
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2015,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BON2,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC4
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    5,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BKEY,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC5
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    13,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_RKEY,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC6
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    6,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_YKEY,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC10
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2011,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_STIM,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC11
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2012,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_MEDI,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC12
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2013,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SOUL,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_INS
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2024,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PINS,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC14
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2025,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SUIT,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC15
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2026,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PMAP,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC16
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL|MF_COUNTITEM,  // flags
    2045,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PVIS,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_CLIP
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2007,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_CLIP,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC17
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2048,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_AMMO,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC18
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2010,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_ROCK,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC19
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2046,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BROK,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC22
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2008,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SHEL,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC23
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2049,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SBOX,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC24
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    8,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_BPAK,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_CHAINGUN
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2002,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_MGUN,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC26
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2005,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_CSAW,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC27
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2003,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_LAUN,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_SHOTGUN
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SPECIAL,  // flags
    2001,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_SHOT,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC31
    0,  // speed
    16*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SOLID,  // flags
    2028,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_COLU,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC43
    0,  // speed
    16*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SOLID,  // flags
    46,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_REDTORCH,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC48
    0,  // speed
    16*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SOLID,  // flags
    48,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_TECHPILLAR,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC49
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    0,  // flags
    34,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_CANDLESTIK,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC50
    0,  // speed
    16*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    MF_SOLID,  // flags
    35,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_CANDELABRA,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC62
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    0,  // flags
    15,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PLAY_DIE7,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC68
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    0,  // flags
    10,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PLAY_XDIE9,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC69
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    0,  // flags
    12,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_PLAY_XDIE9,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
  {  // MT_MISC71
    0,  // speed
    20*FRACUNIT,  // radius
    16*FRACUNIT,  // height
    0,  // flags
    24,  // doomednum
    1000,  // spawnhealth
    8,  // reactiontime
    100,  // mass
    0,  // damage
    S_GIBS,  // spawnstate
    S_NULL,  // seestate
    S_NULL,  // painstate
    S_NULL,  // meleestate
    S_NULL,  // missilestate
    S_NULL,  // deathstate
    S_NULL,  // xdeathstate
    0,  // painchance
    sfx_None,  // seesound
    sfx_None,  // attacksound
    sfx_None,  // painsound
    sfx_None,  // deathsound
    sfx_None,  // activesound
  },
};
