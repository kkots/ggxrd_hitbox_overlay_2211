#pragma once
#include "pch.h"
#include "characterTypes.h"
#include "Hitbox.h"
#include <intrin.h>
#include <limits.h>

const int aswEnginePawnsOffset = 4 + 0x1c4b6c;

using getPos_t = int(__thiscall*)(const void*);
using getPushboxCoords_t = int(__thiscall*)(const void*, int*, int*, int*, int*);
using getExtraTensionModifier_t = int(__thiscall*)(const void*, int param1);

enum HitboxType : char {
	HITBOXTYPE_HURTBOX,
	HITBOXTYPE_HITBOX,
	HITBOXTYPE_EX_POINT,  // used in some animations like Millia Bad Moon
	HITBOXTYPE_EX_POINT_EXTENDED,
	HITBOXTYPE_TYPE4,
	HITBOXTYPE_PUSHBOX,  // only affects horizontal extents
	HITBOXTYPE_TYPE6,
	HITBOXTYPE_NECK,
	HITBOXTYPE_ABDOMEN,
	HITBOXTYPE_R_LEG,
	HITBOXTYPE_L_LEG,
	HITBOXTYPE_PRIVATE0,  // Bedman: boomerang head calculates its distance to this point of Bedman
	HITBOXTYPE_PRIVATE1,
	HITBOXTYPE_PRIVATE2,
	HITBOXTYPE_PRIVATE3,
	HITBOXTYPE_TYPE15,
	HITBOXTYPE_TYPE16
};

struct EntityState {
	bool strikeInvuln;
	bool throwInvuln;
	bool superArmorActive;
	bool isASummon;
	CharacterType charType;
	CharacterType ownerCharType;
	char team;
	bool counterhit;
	bool doingAThrow;
	bool isGettingThrown;
	unsigned int flagsField;
	bool inHitstunBlockstun;
	int posX;
	int posY;
};

struct WakeupTimings {
	int faceUp = 0;
	int faceDown = 0;
};

enum CmnActIndex {
	CmnActStand = 0x0,  // standing
	CmnActStandTurn = 0x1,  // turning around
	CmnActStand2Crouch = 0x2,  // switching from standing to crouching animation
	CmnActCrouch = 0x3,  // crouching
	CmnActCrouchTurn = 0x4,  // turning around while crouching
	CmnActCrouch2Stand = 0x5,  // switching from crouching to standing animation
	CmnActJumpPre = 0x6,  // prejump
	CmnActJump = 0x7,  // jumping
	CmnActJumpLanding = 0x8,  // landing from a jump
	CmnActLandingStiff = 0x9,  // landing recovery after doing an air move, like Ky j.D
	CmnActFWalk = 0xa,  // walking forward
	CmnActBWalk = 0xb,  // walking backwards
	CmnActFDash = 0xc,  // running
	CmnActFDashStop = 0xd,  // stopping running
	CmnActBDash = 0xe,  // backdash
	CmnActAirFDash = 0xf,  // airdashing forward
	CmnActAirBDash = 0x10,  // airdashing backward
	CmnActNokezoriHighLv1 = 0x11,  // these are all hit animations
	CmnActNokezoriHighLv2 = 0x12,
	CmnActNokezoriHighLv3 = 0x13,
	CmnActNokezoriHighLv4 = 0x14,
	CmnActNokezoriHighLv5 = 0x15,
	CmnActNokezoriLowLv1 = 0x16,
	CmnActNokezoriLowLv2 = 0x17,
	CmnActNokezoriLowLv3 = 0x18,
	CmnActNokezoriLowLv4 = 0x19,
	CmnActNokezoriLowLv5 = 0x1a,
	CmnActNokezoriCrouchLv1 = 0x1b,
	CmnActNokezoriCrouchLv2 = 0x1c,
	CmnActNokezoriCrouchLv3 = 0x1d,
	CmnActNokezoriCrouchLv4 = 0x1e,
	CmnActNokezoriCrouchLv5 = 0x1f,
	CmnActBDownUpper = 0x20,  // knocked down backwards, moving upwards
	CmnActBDownUpperEnd = 0x21,  // seems to be unused
	CmnActBDownDown = 0x22,  // knocked down backwards, moving downwards
	CmnActBDownBound = 0x23,  // landing on the back
	CmnActBDownLoop = 0x24,  // lying on the back
	CmnActBDown2Stand = 0x25,  // standing up from having been lying on their back
	CmnActFDownUpper = 0x26,  // knocked down forward, moving upwards
	CmnActFDownUpperEnd = 0x27,  // seems to be unused
	CmnActFDownDown = 0x28,  // knocked down forward, moving downwards
	CmnActFDownBound = 0x29,  // landing on their face
	CmnActFDownLoop = 0x2a,  // lying on their face
	CmnActFDown2Stand = 0x2b,  // standing up from their face
	CmnActVDownUpper = 0x2c,  // launched into air, going up
	CmnActVDownUpperEnd = 0x2d,  // seems to be unused
	CmnActVDownDown = 0x2e,  // launched into air, is at the top of the trajectory or going down
	CmnActVDownBound = 0x2f,  // landing from air launch
	CmnActVDownLoop = 0x30,  // laying down (on the face) from having been launched into the air. Followed by CmnActFDown2Stand
	CmnActBlowoff = 0x31,  // getting hit by Sol 6P, pilebunker
	CmnActKirimomiUpper = 0x32,  // got hit by 5D
	CmnActWallBound = 0x33,  // bouncing from a wall
	CmnActWallBoundDown = 0x34,  // falling down after bouncing from a wall
	CmnActWallHaritsuki = 0x35,  // wallsplat from 5D6/other moves, and wallslump animation
	CmnActWallHaritsukiLand = 0x36,  // landing on the ground from wallslump, also laying/sitting there
	CmnActWallHaritsukiGetUp = 0x37,  // getting up after wallslump, followed by crouch2stand (not part of haritsuki getup)
	CmnActJitabataLoop = 0x38,  // stagger (as in from Ky 5H CH) animation
	CmnActKizetsu = 0x39,  // dizziness animation. You may get a jitabata first, which then transitions into kizetsu
	CmnActHizakuzure = 0x3a,  // crumple from getting hit by max charge blitz. Is followed by CmnActFDownLoop
	CmnActKorogari = 0x3b,  // tumbling animation
	CmnActZSpin = 0x3c,  // happens when Haehyun airthrows you, you spin in the air around the axis perpendicular to the screen
	CmnActUkemi = 0x3d,  // airtech (air recovery)
	CmnActMidGuardPre = 0x3e,  // seems to be unused
	CmnActMidGuardLoop = 0x3f,  // mid proximity blocking or already blocking, in block histop/blockstun
	CmnActMidGuardEnd = 0x40,  // just left blockstun or leaving proximity block animation
	CmnActHighGuardPre = 0x41,  // seems to be unused
	CmnActHighGuardLoop = 0x42,  // overhead proximity blocking or already blocking, in block histop/blockstun
	CmnActHighGuardEnd = 0x43,  // just left blockstun or leaving proximity block animation
	CmnActCrouchGuardPre = 0x44,  // seems to be unused
	CmnActCrouchGuardLoop = 0x45,  // crouch proximity blocking or already blocking, in block histop/blockstun
	CmnActCrouchGuardEnd = 0x46,  // just left crouch blockstun or leaving proximity block animation
	CmnActAirGuardPre = 0x47,  // seems to be unused
	CmnActAirGuardLoop = 0x48,  // air blocking, in block histop/blockstun
	CmnActAirGuardEnd = 0x49,  // just left air blockstun
	CmnActHajikareStand = 0x4a,  // rejected by blitz, also throw break
	CmnActHajikareCrouch = 0x4b,  // rejected by blitz
	CmnActHajikareAir = 0x4c,  // rejected by blitz, also air throw break
	CmnActAirTurn = 0x4d,  // turning in the air
	CmnActLockWait = 0x4e,  // about to get ground/air grabbed, grab already registered, and also being grabbed/participating in grab animation as the victim
	CmnActLockReject = 0x4f,  // don't know
	CmnActAirLockWait = 0x50,  // don't know
	CmnActAirLockReject = 0x51,  // don't know
	CmnActItemUse = 0x52,  // don't know, MOM probably
	CmnActBurst = 0x53,  // burst
	CmnActRomanCancel = 0x54,  // YRC, PRC, RRC
	CmnActEntry = 0x55,  // don't know
	CmnActRoundWin = 0x56,  // round win
	CmnActMatchWin = 0x57,  // match victory animation
	CmnActLose = 0x58,  // lose round due to timeout
	CmnActResultWin = 0x59,  // static pose that happens on victory quote screen
	CmnActResultLose = 0x5a,  // static pose that happens on victory quote screen
	CmnActEntryWait = 0x5b,  // you're invisible
	CmnActExDamage = 0x5c,  // getting hit by Elphelt 236236K or Venom throw
	CmnActExDamageLand = 0x5d,  // Faust 236236S cup shuffling
	NotACmnAct = 0xffffffff
};

enum HitResult {
	HIT_RESULT_NONE,
	HIT_RESULT_NORMAL,
	HIT_RESULT_BLOCKED,
	HIT_RESULT_IGNORED,
	HIT_RESULT_ARMORED,  // or rejected
	HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION
};

enum AttackType {
	ATTACK_TYPE_NONE,
	ATTACK_TYPE_NORMAL,
	ATTACK_TYPE_EX,
	ATTACK_TYPE_OVERDRIVE,
	ATTACK_TYPE_IK
};

enum SuperArmorType {
	SUPER_ARMOR_DODGE,
	SUPER_ARMOR_ARMOR
};

enum MoveCharacterState {
	MOVE_CHARACTER_STATE_NONE,
	MOVE_CHARACTER_STATE_STANDING,
	MOVE_CHARACTER_STATE_CROUCHING,
	MOVE_CHARACTER_STATE_JUMPING
};

enum MoveCondition {
	MOVE_CONDITION_NONE = 0,  // this move condition is always fulfilled
	MOVE_CONDITION_LAND = 1,
	MOVE_CONDITION_AIR = 2,
	MOVE_CONDITION_REQUIRES_50_TENSION = 3,
	MOVE_CONDITION_REQUIRES_100_TENSION = 4,
	MOVE_CONDITION_REQUIRES_25_TENSION = 5,
	// PLAYERVAL_0 != 0 means:
	// For Answer: on scroll
	// For Baiken: Azami
	// For Chipp: in invisibility state from Tsuyoshishiki Meisei
	// For Elphelt: in shotgun mode
	// For I-No: held 3 while hoverdashing for at least 1 frame at some point. Only set after some initial portion of the hoverdash is over
	// For Jam: number of Ryuujins, Gekirins and Kenroukakus remaining (PLAYERVAL_0 set to 2, 1, or 0)
	// For Jack-O: is holding a house
	// For Ramlethal: S sword is on her
	// For Sol: is in DI
	// For Zato: Eddie is summoned
	MOVE_CONDITION_PLAYERVAL_0_TRUE = 0xc,  // also means PLAYERVAL_0 != 0
	// For Ramlethal: S sword is somewhere out there and able to be used
	MOVE_CONDITION_PLAYERVAL_1_TRUE = 0xd,
	// For Ramlethal: H sword is on her
	MOVE_CONDITION_PLAYERVAL_2_TRUE = 0xe,
	// For Ramlethal: H sword is somewhere out there and able to be used
	// For Zato: shadow puddle X. 3000000 means there's no shadow puddle
	MOVE_CONDITION_PLAYERVAL_3_TRUE = 0xf,
	MOVE_CONDITION_PLAYERVAL_0_FALSE = 0x10,
	// PLAYERVAL_1 != 0 means:
	// For Slayer: Bloodsucking Universe powerup
	// For Chipp: already used wallcling during airborne state. Gets reset only when landing
	// For Elphelt: shotgun reached max charge
	// For Jack-O: a non-airborne house is in range
	// For Zato: Eddie gauge disabled
	MOVE_CONDITION_PLAYERVAL_1_FALSE = 0x11,
	MOVE_CONDITION_PLAYERVAL_2_FALSE = 0x12,
	MOVE_CONDITION_PLAYERVAL_3_FALSE = 0x13,
	MOVE_CONDITION_IS_TOUCHING_WALL = 0x3c,
	// GAUGE means RESOURCE, just to clarify
	// For Elphelt: EX_GAUGE_0 is her resource which is the grenade timer
	// For Jam: Gauge 0 is Ryuujin (K card charge), 1 Gekirin (S card charge), 2 Kenroukaku (H card charge)
	// For Johnny: it's his coins
	MOVE_CONDITION_EX_GAUGE_0_EMPTY = 0x3e,
	MOVE_CONDITION_EX_GAUGE_1_EMPTY = 0x3f,
	MOVE_CONDITION_EX_GAUGE_2_EMPTY = 0x40,
	MOVE_CONDITION_EX_GAUGE_3_EMPTY = 0x41,
	MOVE_CONDITION_EX_GAUGE_4_EMPTY = 0x42,
	MOVE_CONDITION_IS_TOUCHING_LEFT_SCREEN_EDGE = 0x44,
	MOVE_CONDITION_IS_TOUCHING_RIGHT_SCREEN_EDGE = 0x45,
    MOVE_CONDITION_ALWAYS_FALSE = 0x46,  // this move condition is never fulfilled
	MOVE_CONDITION_EX_GAUGE_0_NOT_EMPTY = 0x48,
	MOVE_CONDITION_EX_GAUGE_1_NOT_EMPTY = 0x49,
	MOVE_CONDITION_EX_GAUGE_2_NOT_EMPTY = 0x4a,
	MOVE_CONDITION_EX_GAUGE_3_NOT_EMPTY = 0x4b,
	MOVE_CONDITION_EX_GAUGE_4_NOT_EMPTY = 0x4c,
	MOVE_CONDITION_IS_BOSS = 0x5e,
	MOVE_CONDITION_IS_CPU = 0x62,
	MOVE_CONDITION_CLOSE_SLASH = 0x66,
	MOVE_CONDITION_FAR_SLASH = 0x67,
	MOVE_CONDITION_LAST = 0x68  // must always be last
};

enum InputType {
	INPUT_END=0,  // looking for INPUT_BOOLEAN_OR? It's 273
	INPUT_HOLD_P=1,  // 0x1
	INPUT_P_STRICT_PRESS=2,  // 0x2
	INPUT_P_STRICT_RELEASE=3,  // 0x3
	INPUT_PRESS_P=4,  // 0x4
	INPUT_NOT_HOLD_P=5,  // 0x5
	INPUT_RELEASE_P=6,  // 0x6
	INPUT_HOLD_K=10,  // 0xa
	INPUT_K_STRICT_PRESS=11,  // 0xb
	INPUT_K_STRICT_RELEASE=12,  // 0xc
	INPUT_PRESS_K=13,  // 0xd
	INPUT_NOT_HOLD_K=14,  // 0xe
	INPUT_RELEASE_K=15,  // 0xf
	INPUT_HOLD_S=19,  // 0x13
	INPUT_S_STRICT_PRESS=20,  // 0x14
	INPUT_S_STRICT_RELEASE=21,  // 0x15
	INPUT_PRESS_S=22,  // 0x16
	INPUT_NOT_HOLD_S=23,  // 0x17
	INPUT_RELEASE_S=24,  // 0x18
	INPUT_HOLD_H=28,  // 0x1c
	INPUT_H_STRICT_PRESS=29,  // 0x1d
	INPUT_H_STRICT_RELEASE=30,  // 0x1e
	INPUT_PRESS_H=31,  // 0x1f
	INPUT_NOT_HOLD_H=32,  // 0x20
	INPUT_RELEASE_H=33,  // 0x21
	INPUT_HOLD_D=37,  // 0x25
	INPUT_D_STRICT_PRESS=38,  // 0x26
	INPUT_D_STRICT_RELEASE=39,  // 0x27
	INPUT_PRESS_D=40,  // 0x28
	INPUT_NOT_HOLD_D=41,  // 0x29
	INPUT_RELEASE_D=42,  // 0x2a
	INPUT_HOLD_TAUNT=46,  // 0x2e
	INPUT_TAUNT_STRICT_PRESS=47,  // 0x2f
	INPUT_TAUNT_STRICT_RELEASE=48,  // 0x30
	INPUT_PRESS_TAUNT=49,  // 0x31
	INPUT_NOT_HOLD_TAUNT=50,  // 0x32
	INPUT_RELEASE_TAUNT=51,  // 0x33
	INPUT_1=55,  // 0x37
	INPUT_4_OR_1_OR_2=56,  // 0x38
	INPUT_NOT_1=63,  // 0x3f
	INPUT_NOT_4_OR_1_OR_2=64,  // 0x40
	INPUT_2=68,  // 0x44
	INPUT_ANYDOWN=69,  // 0x45
	INPUT_ANYDOWN_STRICT_PRESS=71,  // 0x47
	INPUT_NOT_2=76,  // 0x4c
	INPUT_NOTANYDOWN=77,  // 0x4d
	INPUT_3=81,  // 0x51
	INPUT_6_OR_3_OR_2=82,  // 0x52
	INPUT_NOT_3=89,  // 0x59
	INPUT_NOT_6_OR_3_OR_2=90,  // 0x5a
	INPUT_4=94,  // 0x5e
	INPUT_ANYBACK=95,  // 0x5f
	INPUT_ANYBACK_STRICT_PRESS=97,  // 0x61
	INPUT_NOT_4=102,  // 0x66
	INPUT_NOTANYBACK=103,  // 0x67
	INPUT_5=107,  // 0x6b
	INPUT_ALWAYS_TRUE_DUPLICATE=108,  // 0x6c
	INPUT_NOT_5=115,  // 0x73
	INPUT_6=120,  // 0x78
	INPUT_ANYFORWARD=121,  // 0x79
	INPUT_ANYFORWARD_STRICT_PRESS=123,  // 0x7b
	INPUT_NOT_6=128,  // 0x80
	INPUT_NOTANYFORWARD=129,  // 0x81
	INPUT_7=133,  // 0x85
	INPUT_4_OR_7_OR_8=134,  // 0x86
	INPUT_NOT_7=141,  // 0x8d
	INPUT_NOT_4_OR_7_OR_8=142,  // 0x8e
	INPUT_8=146,  // 0x92
	INPUT_ANYUP=147,  // 0x93
	INPUT_ANYUP_STRICT_PRESS=149,  // 0x95
	INPUT_NOT_8=154,  // 0x9a
	INPUT_NOTANYUP=155,  // 0x9b
	INPUT_9=159,  // 0x9f
	INPUT_6_OR_9_OR_8=160,  // 0xa0
	INPUT_NOT_9=167,  // 0xa7
	INPUT_NOT_6_OR_9_OR_8=168,  // 0xa8
	INPUT_236=172,  // 0xac
	INPUT_623=173,  // 0xad
	INPUT_214=174,  // 0xae
	INPUT_41236=175,  // 0xaf
	INPUT_421=176,  // 0xb0
	INPUT_63214=177,  // 0xb1
	INPUT_236236=178,  // 0xb2
	INPUT_214214=179,  // 0xb3
	INPUT_4123641236=180,  // 0xb4
	INPUT_6321463214=181,  // 0xb5
	INPUT_632146=182,  // 0xb6
	INPUT_641236=183,  // 0xb7
	INPUT_2141236=184,  // 0xb8
	INPUT_2363214=185,  // 0xb9
	INPUT_22=186,  // 0xba
	INPUT_46=187,  // 0xbb
	INPUT_CHARGE_BACK_FORWARD_30F=188,  // 0xbc
	INPUT_CHARGE_DOWN_UP_30F=189,  // 0xbd
	INPUT_6428=190,  // 0xbe
	INPUT_CHARGE_BACK_UP_30F=191,  // 0xbf
	INPUT_64641236=192,  // 0xc0
	INPUT_342646=193,  // 0xc1
	INPUT_28=194,  // 0xc2
	INPUT_646=195,  // 0xc3
	INPUT_P_MASH=196,  // 0xc4
	INPUT_K_MASH=197,  // 0xc5
	INPUT_S_MASH=198,  // 0xc6
	INPUT_H_MASH=199,  // 0xc7
	INPUT_D_MASH=200,  // 0xc8
	INPUT_CIRCLE=201,  // 0xc9
	INPUT_222=204,  // 0xcc
	INPUT_2222=205,  // 0xcd
	INPUT_236236_STRICTER=206,  // 0xce
	INPUT_PRESS_D_DUPLICATE=212,  // 0xd4
	INPUT_HOLD_6_OR_3_AND_PRESS_TWO_OF_PKSH=213,  // 0xd5
	INPUT_ROMAN_CANCEL=214,  // 0xd6
	INPUT_SUPERJUMP=215,  // 0xd7
	INPUT_ANYUP_STRICT_PRESS_DUPLICATE=216,  // 0xd8
	INPUT_FORWARD_DASH=217,  // 0xd9
	INPUT_BACKDASH=218,  // 0xda
	INPUT_BLOCK_WITH_CROSSUP_PROTECTION=219,  // 0xdb
	INPUT_BLOCK_OR_CROSSUP_AIR_BLOCK=220,  // 0xdc
	INPUT_151=221,  // 0xdd
	INPUT_252=222,  // 0xde
	INPUT_353=223,  // 0xdf
	INPUT_454=224,  // 0xe0
	INPUT_66_QUICK=225,  // 0xe1
	INPUT_757=226,  // 0xe2
	INPUT_858=227,  // 0xe3
	INPUT_959=228,  // 0xe4
	INPUT_ALWAYS_TRUE_DUPLICATE2=233,  // 0xe9
	INPUT_ALWAYS_FALSE_DUPLICATE=234,  // 0xea
	INPUT_623_LENIENT=235,  // 0xeb
	INPUT_22_LENIENT=238,  // 0xee
	INPUT_5_OR_4_OR_ANY_UP=239,  // 0xef
	INPUT_5_OR_ANY_UP=240,  // 0xf0
	INPUT_5_OR_6_OR_ANY_UP=241,  // 0xf1
	INPUT_5_OR_4_OR_7_OR_8=242,  // 0xf2
	INPUT_421_LENIENT=243,  // 0xf3
	INPUT_16243=246,  // 0xf6
	INPUT_546=247,  // 0xf7
	INPUT_5_ANYBACK_ANYFORWARD_STRICTER=249,  // 0xf9
	INPUT_5_ANYFORWARD_ANYBACK=250,  // 0xfa
	INPUT_CHARGE_BACK_FORWARD_40F=251,  // 0xfb
	INPUT_CHARGE_DOWN_UP_40F=252,  // 0xfc
	INPUT_CHARGE_BACK_FORWARD_45F=253,  // 0xfd
	INPUT_CHARGE_DOWN_UP_45F=254,  // 0xfe
	INPUT_236236236=255,  // 0xff
	INPUT_623_WITHIN_LAST_3F=256,  // 0x100
	INPUT_5_ANYBACK_ANYFORWARD_WITHIN_LAST_2F=257,  // 0x101
	INPUT_NOTANYDOWN_2=258,  // 0x102
	INPUT_46_WITHIN_LAST_1F=259,  // 0x103
	INPUT_CHARGE_DOWN_10F=260,  // 0x104
	INPUT_546_BUTNOT_54_ANYDOWN_6=261,  // 0x105
	INPUT_5_ANYBACK_ANYFORWARD_LENIENT=262,  // 0x106
	INPUT_BURST=268,  // 0x10c
	INPUT_HOLD_TWO_OR_MORE_OF_PKSH=269,  // 0x10d
	INPUT_PRESS_TWO_OR_MORE_OF_PKSH_GLITCHED=270,  // 0x10e
	INPUT_PRESS_ANYBACK_WITHIN_LAST_8F_NO_MASH_ALLOWED=271,  // 0x10f
	INPUT_P_OR_K_OR_S_OR_H=272,  // 0x110
	INPUT_BOOLEAN_OR=273,  // 0x111
	INPUT_HAS_PRECEDING_5=274,  // 0x112
	INPUT_ITEM=275,  // 0x113
	INPUT_HOLD_SPECIAL=276,  // 0x114
	INPUT_SPECIAL_STRICT_PRESS=277,  // 0x115
	INPUT_SPECIAL_STRICT_RELEASE=278,  // 0x116
	INPUT_PRESS_SPECIAL=279,  // 0x117
	INPUT_NOT_HOLD_SPECIAL=280,  // 0x118
	INPUT_RELEASE_SPECIAL=281,  // 0x119
	INPUT_ANY_TWO_OF_PKSH=285,  // 0x11d
	INPUT_ROMAN_CANCEL_DUPLICATE=286,  // 0x11e
	INPUT_MOM_TAUNT=287,  // doubt, 0x11f
	INPUT_FORWARD_DASH_WITHIN_LAST_2F=288,  // 0x120
	INPUT_BACKDASH_WITHIN_LAST_2F=289,  // 0x121
	INPUT_P_OR_K_OR_S_OR_H_OR_D_STRICT_PRESS=290,  // 0x122
	INPUT_ALWAYS_FALSE=291,  // 0x123
	INPUT_PRESS_TAUNT_DUPLICATE=292,  // 0x124
	INPUT_HOLD_ONE_OR_MORE_OF_PKSH=293,  // 0x125
	INPUT_HOLD_ONE_OR_MORE_OF_PKSH_OR_D=294,  // 0x126
};

enum MoveSubcategory {
    MOVE_SUBCATEGORY_DEFAULT,
    MOVE_SUBCATEGORY_TAUNT,
    MOVE_SUBCATEGORY_RESPECT,
    MOVE_SUBCATEGORY_ITEM_USE,
    MOVE_SUBCATEGORY_BURST_OVERDRIVE,
    MOVE_SUBCATEGORY_BAIKEN_AZAMI
};

enum MoveType {
	MOVE_TYPE_NEUTRAL = 0,
	MOVE_TYPE_FORWARD_WALK = 1,
	MOVE_TYPE_BACKWARD_WALK = 2,
	MOVE_TYPE_FORWARD_DASH = 3,
	MOVE_TYPE_BACKWARD_DASH = 4,
	MOVE_TYPE_FORWARD_JUMP = 5,
	MOVE_TYPE_BACKWARD_JUMP = 6,
	MOVE_TYPE_NEUTRAL_JUMP = 7,
	MOVE_TYPE_FORWARD_SUPER_JUMP = 8,
	MOVE_TYPE_BACKWARD_SUPER_JUMP = 9,
	MOVE_TYPE_NEUTRAL_SUPER_JUMP = 10,
	MOVE_TYPE_FORWARD_AIR_JUMP = 11,
	MOVE_TYPE_BACKWARD_AIR_JUMP = 12,
	MOVE_TYPE_NEUTRAL_AIR_JUMP = 13,
	MOVE_TYPE_FORWARD_AIR_DASH = 14,
	MOVE_TYPE_BACKWARD_AIR_DASH = 15,
	MOVE_TYPE_DUST_FOLLOWUP = 16,
	MOVE_TYPE_NORMAL = 17,
	MOVE_TYPE_SPECIAL = 18,
	MOVE_TYPE_MIST_CANCEL = 19,
	MOVE_TYPE_OVERDRIVE = 20,
	MOVE_TYPE_INSTANT_KILL = 21,
	MOVE_TYPE_DEAD_ANGLE_ATTACK = 22,
	MOVE_TYPE_BLUE_BURST = 23,
	MOVE_TYPE_GOLD_BURST = 24,
	MOVE_TYPE_E_MOVE = 25,
	MOVE_TYPE_FAULTLESS_DEFENCE = 26,
	MOVE_TYPE_ROMAN_CANCEL_YELLOW = 27,
	MOVE_TYPE_ROMAN_CANCEL_REDPURPLE = 28,  // I think all Roman Cancels are just this type instead
	MOVE_TYPE_INSTANT_KILL_PREPARATION = 29,
	MOVE_TYPE_BLITZ_SHIELD = 30
};

template<size_t size>
struct BitArrayIterator {
	const DWORD* valuesPtr;
	size_t index;
	DWORD currentDword;
	int bitIndex;
	BitArrayIterator<size> operator++() {
		getNext();
		return *this;
	}
	BitArrayIterator<size> operator++(int) {
		BitArrayIterator<size> copy = *this;
		getNext();
		return copy;
	}
	void getNext() {
		if (index >= size) {
			bitIndex = -1;
			return;
		}
		DWORD bitScan;
		while (true) {
			if (currentDword && _BitScanForward(&bitScan, currentDword)) {
				currentDword &= ~(1 << bitScan);
				bitIndex = (int)(32 * index + bitScan);
				return;
			}
			++index;
			if (index >= size) {
				bitIndex = -1;
				return;
			}
			currentDword = valuesPtr[index];
		}
	}
	bool operator==(const BitArrayIterator& other) const {
		return valuesPtr == other.valuesPtr
			&& index == other.index
			&& currentDword == other.currentDword;
	}
	bool operator!=(const BitArrayIterator& other) const {
		return !(*this == other);
	}
	int operator*() const {
		return bitIndex;
	}
};

template<size_t size>
struct BitArray {
	DWORD values[size];
	bool getBit(DWORD index) const {
		DWORD bitMask = 1 << (index & 31);
		return (values[index >> 5] & bitMask) != 0;
	}
	bool setBit(DWORD index) {
		DWORD bitMask = 1 << (index & 31);
		return values[index >> 5] |= bitMask;
	}
	bool removeBit(DWORD index) {
		DWORD bitMask = 1 << (index & 31);
		return values[index >> 5] &= ~bitMask;
	}
	BitArrayIterator<size> begin() const {
		BitArrayIterator<size> result;
		result.valuesPtr = values;
		result.index = 0;
		result.currentDword = values[0];
		result.getNext();
		return result;
	}
	BitArrayIterator<size> end() const {
		BitArrayIterator<size> result;
		result.valuesPtr = values;
		result.index = size;
		result.currentDword = 0;
		return result;
	}
};

struct AddedMoveData {
	int index;
	int stylishRealMoveIndex;  // for stylish moves: realMoveIndex. Non-stylish: matches index
	char name[32];
	char stylishRealMoveName[32];
	char stateName[32];  // state function name
	unsigned int requestActionFlag; // bitfield
	MoveType type;
	MoveSubcategory subcategory;
	MoveCharacterState characterState;
	DWORD flags0x78;
	inline bool isFollowupMove() const { return (flags0x78 & 2) != 0; }
	DWORD cpuEstimateFlags;
	DWORD inputDirectionBaseObject;
	BitArray<5> conditions;
	InputType inputs[16];
	char moveRecipeEMove[32];
	int recipeStopTypeExArg3;
	int recipeStopTypeExArg2;
	int recipeStopTypeExArg1;
	DWORD eMoveOffset;
	DWORD eMoveDataSize;
	int padding[4];
	BYTE priority;
	char padding2;
	char shareChain[32];
	char padding3[2];
	int enemyHpRate;
	char callSubroutine[32];
	char conditionCheckSubroutine[32];
	int minimumHeightRequirement;
	int maxXDistanceToEnemy;  // used by stylish combos
	int minYDistanceToEnemyAbove;  // used by stylish combos
	int maxYDistanceToEnemyAbove;  // used by stylish combos
	DWORD forceDisableFlags;
	DWORD hitOrBlockstunCondition;  // the function that uses this in the game may return 2 or 1 or 0. 2 means nope, 1 means ok. Not sure what 0 is for
	int cpuEstimatePoint;
	int padding4[11];
	DWORD flags0x1cc;
	inline bool gatlingOption() const { return (flags0x1cc & 0x1) != 0; }
	inline bool whiffCancelOption() const { return (flags0x1cc & 0x10) != 0; }
	inline bool passedAllChecks() const { return (flags0x1cc & 0x80) != 0; }
	char padding5[4];
	int frameCounterPerFacing[2];  // gets incremented every frame until reaches 10000
	int frameCounterPerFacingNoHitstop[2];  // when not in hitstop or superfreeze or frame lost due to RC slowdown, gets incremented every frame until reaches 10000
	int passedEnabledFlags;
	int padding6;
	BOOL passedMoveChecksOfSomeKind;
	int padding7;
	int bufferTime;
	int padding8;
	inline bool hasCondition(MoveCondition condition) const { return conditions.getBit(condition); }
	inline void addCondition(MoveCondition condition) { conditions.setBit(condition); }
	inline void removeCondition(MoveCondition condition) { conditions.removeBit(condition); }
};

enum CounterHitType {
	COUNTERHIT_TYPE_DEFAULT,
	COUNTERHIT_TYPE_NO_COUNTER,
	COUNTERHIT_TYPE_FORCE_COUNTER
};

enum KillType {
	KILL_TYPE_NORMAL,
	KILL_TYPE_NOT_KILL,
	KILL_TYPE_NOT_KILL_IN_COMBO,
	KILL_TYPE_KILL,
	KILL_TYPE_KILL_ALLY
};

enum GuardType {
	GUARD_TYPE_ANY,
	GUARD_TYPE_HIGH,
	GUARD_TYPE_LOW,
	GUARD_TYPE_NONE
};

struct AttackData {
	AttackType type;
	int level;
	int damage;
	DWORD flags0xc;
	inline bool hellfire() const { return (flags0xc & 0x10000) != 0; }
	inline bool enableGuardBreak() const { return (flags0xc & 0x40000) != 0; }
	inline bool airUnblockable() const { return (flags0xc & 0x100000) != 0; }
	inline bool easyClash() const { return (flags0xc & 0x1000000) != 0; }
	DWORD flags0x10;
	inline bool scaleDamageWithHp() const { return (flags0x10 & 0x1) != 0; }
	inline bool throwLockExecute() const { return (flags0x10 & 0x4) != 0; }
	inline bool ignoreWeight() const { return (flags0x10 & 0x8) != 0; }  // noGravityScaling
	inline bool noHitstunScaling() const { return (flags0x10 & 0x10) != 0; }
	inline bool dontUseComboTimerForPushback() const { return (flags0x10 & 0x20) != 0; }  // noPushbackScaling
	inline bool ignoreOtg() const { return (flags0x10 & 0x40) != 0; }
	inline bool noDustScaling() const { return (flags0x10 & 0x80) != 0; }
	inline bool noDamageScaling() const { return (flags0x10 & 0x100) != 0; }
	inline bool dontUseComboTimerForSpeedY() const { return (flags0x10 & 0x400) != 0; }
	inline bool prorationTandan() const { return (flags0x10 & 0x10000) != 0; }  // this flag is always set on everything, even when not attacking
	inline bool isThrow() const { return (flags0x10 & 0x800000) != 0; }
	DWORD flags0x14;
	inline bool canGrab() const { return (flags0x14 & 1) != 0; }
	inline bool collisionForceExpand() const { return (flags0x14 & 0x4) != 0; }
	inline bool onlyHitGround() const { return (flags0x14 & 0x10) != 0; }
	inline bool onlyHitAir() const { return (flags0x14 & 0x20) != 0; }
	inline bool whiffCrouch() const { return (flags0x14 & 0x40) != 0; }
	inline bool clashOnly() const { return (flags0x14 & 0x1000000) != 0; }  // RTL RideAura has this, Jack O' "Aigisfield" (sic)
	inline bool attackMultiHit() const { return (flags0x14 & 0x4000000) != 0; }  // Elphelt grenade, Elphelt shotgun, Oil Fire, 100-t Weight, Stahl Wirbel, etc. Lets you hit multiple entities in one frame
	inline bool wasHitDuringRc() const { return (flags0x14 & 0x20000000) != 0; }
	DWORD flags0x18;
	inline bool dustAttack() const { return (flags0x18 & 0x4) != 0; }
	int extraHitstops[3];  // [2] is extra CH hitstop
	int hitstop;
	int attackLockWaitTime;
	int blockstun;
	int blockstunAirExtra;
	int throwMaxX;
	int throwMaxY;
	int throwMinX;
	int throwMinY;
	int throwRange;
	int projectileLvl;
	CounterHitType counterHitType;
	char attackLockAction[32];
	DWORD undefined1;
	char undefined2[32];
	DWORD guardEffectParam1;
	char guardEffect[32];
	int damageToFaultlessDefense;
	DWORD undefined6;
	DWORD attackFrontDirection;
	int undefined7;
	KillType killType;
	char nextHitActionName[32];
	int attackFinishBackground;
	int attackHitPositionType_param1;
	int attackHitPositionType_param2;
	int attackHitPositionType_param3;
	int varAdd; // attackExHitParam
	DWORD varEntity;
	DWORD varFlags;
	DWORD varTag;
	DWORD varId;
	int varMin;
	int varMax;
	int poisonDuration;
	int poisonPercentage;
	DWORD throwRejectType;
	int minimumDamagePercent;
	int stun;
	GuardType guardType;
	char attackLockSprite[32];
	int pushbackModifier;  // Ky 6K has this set to 65 and bbscript calls it hitPushbackX
	int pushbackModifierOnHitstun;  // pushbackXForHit
	int extraCrouchHitstun;  // crouchHitstunAddition
	int airHitstunFromLandAddition;
	int angle;
	int staggerDuration;
	int tensionGainOnConnect;
	char hitSoundEffect[32];
	char undefined24[32];
	char guardSoundEffect[32];
	int atkLevelOnBlockOrArmor;
	int riscPlus;
	int riscMinus;
	int initialProration;
	int forcedProration;
	int riscMinusStarter;
	int riscMinusOnce;
	int attackKezuri;  // chip damage in units of 1/128. By default 16 for specials and supers, 0 for normals, which stands for 12,5%
	int unburstableTime;
	int invulnTime;
	int dustInvulnTime;
	int damageSprite;
	int lifeStealPercent;
	int exDamageType;
	char trialName[32];
	int airPushbackModifier;  // Has not been spotted yet to not be 0
	DWORD undefined26;
};

enum TensionMode {
	TENSION_MODE_NORMAL,
	TENSION_MODE_RED_IK_ACTIVE,
	TENSION_MODE_GOLD_IK_ACTIVE,
	TENSION_MODE_DISABLED_UNTIL_END_OF_ROUND
};

enum CounterHitEntityValue {
	COUNTER_HIT_ENTITY_VALUE_NO_COUNTERHIT,
	COUNTER_HIT_ENTITY_VALUE_HIT_AFTER_COUNTERHIT,  // seems to be followup hit after a counterhit, but this value was never oberserved to stay at the end of a tick. It gets changed to something else
	COUNTER_HIT_ENTITY_VALUE_COUNTERHIT,
	COUNTER_HIT_ENTITY_VALUE_MORTAL_COUNTER
};

enum RomanCancelAvailability {
	ROMAN_CANCEL_DISALLOWED,
	ROMAN_CANCEL_ALLOWED,
	ROMAN_CANCEL_DISALLOWED_WITH_X_MARK,
	ROMAN_CANCEL_DISALLOWED_ON_WHIFF_WITH_X_MARK
};

struct BBScrHashtableElement {
	void* vtable;
	DWORD hashValue;
	int addressInCommands;
};

struct BBScrHashtable {
	void* vtable;
	BBScrHashtableElement* ptr;
	size_t currentSize;
	size_t maxSize;
};

struct BBScrInfo {
	BYTE* base;
	BYTE* afterJumptable;
	DWORD jumptableEntryCount;
	BYTE* jumptableStart;
	BBScrHashtable* stateMap;
	BBScrHashtable* subroutineMap;
};

enum TeamSwap {
	TEAM_SWAP_NORMAL,
	TEAM_SWAP_OPPOSITE,
	TEAM_SWAP_NEITHER
};

struct UponInfo {
	BYTE* uponInstrPtr;  // used if bbscrNeedExecuteUpon (hasUpon) is set
	char stateToGoTo[32];  // used if bbscrNeedGoToStateUpon (needGoToStateUpon) is set. Name of anim
	int fireOnThisValue;  // for upon 0xd it's frame number, for upon 0x4 it's speedY
	int fireOnThisValue2;
	char markerToGoTo[32];  // used if bbscrNeedGoToMarkerUpon (needGoToMarkerUpon) is set. Marker name
	DWORD flags;
};

struct ScheduledAnim {
	char animName[32];
	DWORD requestActionFlag;
	inline bool isPerformedRaw() const { return (requestActionFlag & 0x2000) != 0; }  // means move was not cancelled into and is performed from neutral stance
	char markerName[32];
	int moveIndex;
	int nonZeroOnAnimChange;
};

// the following several bbscript-related enums are mostly taken or derived from information in https://github.com/super-continent/ggxrd-mod.
// In its tree, see static_db/ggrev2.ron
enum EntityReferenceType {
	ENT_NONE = 0x0,
	ENT_PREVIOUS = 0x1,
	ENT_PARENT = 0x2,
	ENT_PLAYER = 0x3,
	ENT_STACK_0 = 0x4,
	ENT_STACK_1 = 0x5,
	ENT_STACK_2 = 0x6,
	ENT_STACK_3 = 0x7,
	ENT_STACK_4 = 0x8,
	ENT_STACK_5 = 0x9,
	ENT_STACK_6 = 0xa,
	ENT_STACK_7 = 0xb,
	ENT_STACK_FOR_CMN_ACT = 0xc,
	ENT_WINNER = 0xd,
	ENT_LOSER = 0xe,
	ENT_LOCKED = 0x15,
	ENT_ENEMY = 0x16,
	ENT_SELF = 0x17,
	ENT_LAST_ATTACKER = 0x18,
	ENT_LAST_DEFENDER = 0x19,
	ENT_STACK_9 = 0x1a,
	ENT_PLAYER_0 = 0x1b,
	ENT_PLAYER_1 = 0x1c
};

enum BBScrEvent {
	BBSCREVENT_IMMEDIATE = 0,
	BBSCREVENT_BEFORE_EXIT = 1,
	BBSCREVENT_LANDING = 2,
	BBSCREVENT_ANIMATION_FRAME_ADVANCED = 3,  // also called Idling
	BBSCREVENT_SPEED_Y_DECREASED_BELOW_TARGET = 4,
	BBSCREVENT_LANDING_REPEATUSE = 5,
	BBSCREVENT_TOUCH_WALL_OR_SCREEN_EDGE = 6,
	BBSCREVENT_STATE_REACHED_END = 7,
	BBSCREVENT_YOUR_ATTACK_COLLISION = 8,
	BBSCREVENT_YOUR_ATTACK_COLLISION_WITH_A_PLAYER = 9,
	BBSCREVENT_HIT_A_PLAYER = 10,
	BBSCREVENT_GOT_HIT = 11,
	BBSCREVENT_FRAMESTEP_1 = 12,
	BBSCREVENT_REACHED_TARGET_ANIM_DURATION = 13,
	BBSCREVENT_DIE = 14,
	BBSCREVENT_NOT_HOLD_P = 15,
	BBSCREVENT_NOT_HOLD_K = 16,
	BBSCREVENT_NOT_HOLD_S = 17,
	BBSCREVENT_NOT_HOLD_H = 18,
	BBSCREVENT_NOT_HOLD_D = 19,
	BBSCREVENT_DISTANCE_TO_PLAYER_LESS_THAN_TARGET = 20,
	BBSCREVENT_FINALIZE = 21,
	BBSCREVENT_CUSTOM_SIGNAL_0 = 23,
	BBSCREVENT_CUSTOM_SIGNAL_1 = 24,
	BBSCREVENT_CUSTOM_SIGNAL_2 = 25,
	BBSCREVENT_CUSTOM_SIGNAL_3 = 26,
	BBSCREVENT_CUSTOM_SIGNAL_4 = 27,
	BBSCREVENT_CUSTOM_SIGNAL_5 = 28,
	BBSCREVENT_CUSTOM_SIGNAL_6 = 29,
	BBSCREVENT_CUSTOM_SIGNAL_7 = 30,
	BBSCREVENT_CUSTOM_SIGNAL_8 = 31,
	BBSCREVENT_CUSTOM_SIGNAL_9 = 32,
	BBSCREVENT_MUTEKI = 33,
	BBSCREVENT_PLAYER_GOT_HIT = 35,
	BBSCREVENT_FRAME_STEP = 36,
	BBSCREVENT_CLASH = 37,
	BBSCREVENT_PLAYER_BLOCKED = 38,
	BBSCREVENT_PRE_DRAW = 39,
	BBSCREVENT_GUARD = 40,
	BBSCREVENT_ARMOR = 41,
	BBSCREVENT_PRE_FRAME_STEP = 42,
	BBSCREVENT_ARMOR2 = 43,
	BBSCREVENT_HIT_OWN_PROJECTILE_DEFAULT = 44,
	BBSCREVENT_DESTROY = 45,
	BBSCREVENT_TOUCH_WALL = 46,
	BBSCREVENT_PLAYER_CHANGED_STATE = 47,
	BBSCREVENT_SEND_SIGNAL_NAME = 49,
	BBSCREVENT_SUPERFREEZE = 50,
	BBSCREVENT_LANDED = 51,
	BBSCREVENT_START = 53,
	BBSCREVENT_BEFORE_ATTACK_PROPERTIES_COPY = 54,
	BBSCREVENT_BEFORE_ATTACK_PROPERTIES_COPY_HIT_BY_TEAMMEMBER = 55,
	BBSCREVENT_FRAME_STEP_AFTER = 56,
	BBSCREVENT_PLAYER_GOT_INSTANT_KILLED = 57,
	BBSCREVENT_OPPONENT_GOT_INSTANT_KILLED = 58,
	BBSCREVENT_INSTANT_KILL = 59,
	BBSCREVENT_NO_GUARD_EFFECT = 60,
	BBSCREVENT_YOUR_ATK_GOT_FD_BLOCKED = 61,
	BBSCREVENT_ENTRY_WAIT = 63,
	BBSCREVENT_ARMOR2_PROJECTILE = 64,
	BBSCREVENT_REFLECT_ENEMY_PROJECTILE = 65,
	BBSCREVENT_GOT_REFLECTED = 66,
	BBSCREVENT_PRE_COLLISION_CHECK = 67,
	BBSCREVENT_ASSARI_GOT_HIT = 68,
	BBSCREVENT_GOT_HIT2 = 69,
	BBSCREVENT_PLAYER_GETTING_HIT = 71,
	BBSCREVENT_PLAYER_BLOCKED_ATTACK = 72,
	BBSCREVENT_ENEMY_GETTING_HIT = 73,
	BBSCREVENT_ENEMY_BLOCKED_ATTACK = 74,
	BBSCREVENT_DECIDE_GUARD_EFFECT = 76,
	BBSCREVENT_FRAME_STEP_AFTER_EXTRA = 77,
	BBSCREVENT_HIT_THE_OPPONENT_PRE_ATK_VALUES_COPY = 78
};

enum BBScrOperation {
	BBSCROP_ADD,  // result = x + y
	BBSCROP_SUB,  // result = x - y
	BBSCROP_MUL,  // result = x * y
	BBSCROP_DIV,  // result = x / y (round down)
	BBSCROP_MOD,  // result = x % y (remainder of division)
	BBSCROP_AND,  // result = x && y (only returns 1 if x != 0 and y != 0, otherwise returns 0)
	BBSCROP_OR,  // result = x || y (returns 1 if x != 0 or y != 0, otherwise returns 0)
	BBSCROP_BIT_AND,  // result = x & y (bitwise AND)
	BBSCROP_BIT_OR,  // result = x | y (bitwise OR)
	BBSCROP_IS_EQUAL,  // result = (x == y) (returns 1 if x == y, otherwise 0)
	BBSCROP_IS_GREATER,  // result = (x > y)
	BBSCROP_IS_LESSER,  // result = (x < y)
	BBSCROP_IS_GREATER_OR_EQUAL,  // result = (x >= y)
	BBSCROP_IS_LESSER_OR_EQUAL,  // result = (x <= y)
	BBSCROP_BIT_NOT,  // result = (x & (~y)) (this bitwise inverts the right argument and then performs a BIT_AND)
	BBSCROP_IS_NOT_EQUAL,  // result = (x != y) (returns 1 if x != y, otherwise 0)
	BBSCROP_MOD_EQUALS_0,  // result = ((x % y) == 0)
	BBSCROP_MOD_EQUALS_1,  // result = ((x % y) == 1)
	BBSCROP_MOD_EQUALS_2,  // result = ((x % y) == 2)
	BBSCROP_ADD_DIR,  // result = x + y * facing (facing is 1 if your graphical sprite is facing right, -1 otherwise)
	BBSCROP_TAKE_RIGHT_VALUE_UNCHANGED,  // result = y
	BBSCROP_SET_DIR,  // result = y * facing (facing is 1 if your graphical sprite is facing right, -1 otherwise)
	BBSCROP_GREATER_THAN_RANDOM,  // result = (x > (rand() % y)) (returns 1 if x > than that, otherwise returns 0. Their rand() function produces numbers from 0 to 4294967295)
};

enum BBScrTag {
	BBSCRTAG_VALUE,  // literal
	BBSCRTAG_VARIABLE=2
};

enum BBScrVariable {
	// You can only write to variables that are marked as "writable". Writing to non-writable variables will have no effect.
	// Variables marked as "per player" allow access from effect scripts, but they access that value through their corresponding player.
	// Variables marked as "global" are stored in the game engine as a single variable, and both players and their effect scripts access the same variable.
    BBSCRVAR_ACCUMULATOR=0,  // writable. Is used to obtain values from 'calcDistance', some if functions, checks, and other common functions
    BBSCRVAR_ROTATION=1,  // writable. In 1/1000'th of degree. 1000 means 1 degree. Positive value rotates counter-clockwise when facing right, and clockwise when facing left. Same as PITCH. Pitch is the angle of rotation around the axis perpendicular to the screen
    BBSCRVAR_IS_CMN_ACT=2,  // returns 1 if the current action is a CmnAct..., otherwise returns 0
    BBSCRVAR_PLAYERVAL_0=3,  // writable, per player. Gets reset to 0 on stage reset or new round. Survives state (animation) change
    BBSCRVAR_PLAYERVAL_1=4,  // writable, per player. Gets reset to 0 on stage reset or new round. Survives state (animation) change
    BBSCRVAR_PLAYERVAL_2=5,  // writable, per player. Gets reset to 0 on stage reset or new round. Survives state (animation) change
    BBSCRVAR_PLAYERVAL_3=6,  // writable, per player. Gets reset to 0 on stage reset or new round. Survives state (animation) change
    BBSCRVAR_PHYSICS_X_IMPULSE=7,  // writable. Your current X speed. If you set this, this sets your speed towards your current facing, so, providing a positive value while facing to the left would assign you negative speed. If you get this value, it gets multiplied by your facing before you see the result, so, if your speed is negative while facing to the left you get read a positive speed.
    BBSCRVAR_PHYSICS_Y_IMPULSE=8,  // writable. Your current Y speed. Positive speed Y means going up, negative going down
    // 9, 10, 11, 12
    BBSCRVAR_FRAMES_PLAYED_IN_STATE=13,  // restarts from 1 on each state. Only advances forward when a sprite frame advances
    BBSCRVAR_OPPONENT_X_DISTANCE=14,  // the absolute value of opponent x - your x
    BBSCRVAR_OPPONENT_Y_DISTANCE=15,  // the absolute value of opponent y - your y
    BBSCRVAR_MATCH_RUNNING=16,  // global
    BBSCRVAR_POS_X=17,  // writable. Your current origin point's X coordinate, sometimes (read on) multiplied by your facing. Origin point is the point between your feet. When you try to set this value, what you set first gets multiplied by your facing, so if you set a negative X while facing left, you would set a positive value due to this. However, the same does not apply when reading this value: you read a negative position if it's negative, and a positive if positive, regardless of which direction you're facing.
    BBSCRVAR_POS_Y=18,  // writable. Your current origin point's Y coordinate. Origin point is the point between your feet
    BBSCRVAR_THIS_IS_ALWAYS_ZERO=19,  // always returns 0, no matter what
    BBSCRVAR_DISTANCE_TO_WALL_IN_FRONT=20,  // means the wall, not screen edge. Equal to abs( X of wall - your X )
    BBSCRVAR_DISTANCE_TO_WALL_IN_FRONT_AGAIN=21,  // same as DISTANCE_TO_WALL_IN_FRONT
    BBSCRVAR_PRIVATE_0_HITBOXES_COUNT=22,  // per player. Returns the number of PRIVATE_0 hitboxes of your corresponding player
    BBSCRVAR_DISTANCE_FROM_THIS_CENTER_TO_ENEMY_CENTER=23,  // center means center of body, it's lifted off the ground a bit. Is equal to sqrt((enemy center X - your center X)^2 + (enemy center Y - your center Y)^2)
    BBSCRVAR_PLAYER_IN_HITSTUN=24,  // per player. Does not include the frame when your player enter hitstun, unless they were already in hitstun on that frame
    BBSCRVAR_RESOURCE_AMOUNT=25,  // writable, per player. This is RESOURCE_AMOUNT_0. You have 5 slots to store different amounts of resources in. Resources get reset to 0 each round and stage reset
    BBSCRVAR_RESOURCE_AMOUNT_1=26,  // writable, per player. Resources get reset to 0 each round and stage reset
    BBSCRVAR_RESOURCE_AMOUNT_2=27,  // writable, per player. Resources get reset to 0 each round and stage reset
    BBSCRVAR_RESOURCE_AMOUNT_3=28,  // writable, per player. Resources get reset to 0 each round and stage reset
    BBSCRVAR_RESOURCE_AMOUNT_4=29,  // writable, per player. Resources get reset to 0 each round and stage reset
    BBSCRVAR_AIRBORNE=30,  // this check if based on your Y coordinate being > 0 and a flag which is not fully understood
    BBSCRVAR_NOT_IN_AIR=31,  // the opposite of AIRBORNE
    BBSCRVAR_FACING_LEFT=32,  // writable. This is the facing used to draw your sprite. This value is 0 if the graphical sprite is facing right and 1 if left
    BBSCRVAR_PLAYER_INPUTS_FACING_LEFT=33,  // writable, per player. This is the facing that is used when parsing inputs. It's 0 if right, 1 if left
    BBSCRVAR_OPPONENT_X_OFFSET=34,  // = opponent x - your x. Can be negative
    BBSCRVAR_OPPONENT_Y_OFFSET=35,  // = opponent y - your y. Can be negative
    BBSCRVAR_PLAYER_IN_HITSTUN_OR_BLOCKSTUN=36,  // per player. Does not include the frame when you enter hitstun or blockstun, unless you were already in hitstun or blockstun on that frame
    BBSCRVAR_VOICE_ID_IS_0=37,  // per player. At the start of a match each character gets assigned a random VoiceID from 0 to 2, maybe except I-No in some situations, where she gets 0 or 1
    BBSCRVAR_VOICE_ID_IS_1=38,  // per player
    BBSCRVAR_VOICE_ID_IS_2=39,  // per player
    BBSCRVAR_VOICE_ID_IS_3=40,  // per player. Voice ID can never be 3
    BBSCRVAR_SEND_SIGNAL_EX_PARAM2=42,  // writable, global. This allows you to write the second parameter of the next sendSignalEx call or read the second parameter of the last sendSignalEx call. Careful: this variable is global, meaning you must use it immediately after setting
    BBSCRVAR_SEND_SIGNAL_EX_PARAM3=43,  // writable, global. This allows you to write the third parameter of the next sendSignalEx call or read the third parameter of the last sendSignalEx call. Careful: this variable is global, meaning you must use it immediately after setting
    BBSCRVAR_DISTANCE_FROM_THIS_CENTER_TO_PLAYER_CENTER=44,  // center means center of body, it's lifted off the ground a bit. Is equal to sqrt((this center X - your player center X)^2 + (this center Y - your player Y)^2)
    // 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60: writable
    BBSCRVAR_GTMP_X=61,  // writable, per player. Gets reset to 0 on stage reset. This value participates in forming the POS_TYPE(GTMP). You can use this as just a variable
    BBSCRVAR_GTMP_Y=62,  // writable, per player. Gets reset to 0 on stage reset. This value participates in forming the POS_TYPE(GTMP). You can use this as just a variable
    // 63, 64, 65, 66, 67, 68: writable, per player
    // The following values survive state (animation) switch
    BBSCRVAR_STORAGE1=69,  // writable. Reset on stage reset or new round
    BBSCRVAR_STORAGE2=70,  // writable. Reset on stage reset or new round
    BBSCRVAR_STORAGE3=71,  // writable. Reset on stage reset or new round
    BBSCRVAR_STORAGE4=72,  // writable. Reset on stage reset or new round
    BBSCRVAR_STORAGE5=73,  // writable. Does not get reset on stage reset
    BBSCRVAR_STORAGE6=74,  // writable. Does not get reset on stage reset
    BBSCRVAR_STORAGE7=75,  // writable. Does not get reset on stage reset
    BBSCRVAR_STORAGE8=76,  // writable. Does not get reset on stage reset
    BBSCRVAR_HEALTH=77,  // writable
    BBSCRVAR_MAX_HEALTH=78,  // writable. Standard value is 420 for all characters. They do differ in effective HP though due to guts, but that is not reflected here
    BBSCRVAR_DAMAGE_SCALE=79,  // writable, per player. Percentage that alters all dealt damage by you or your projectiles
    BBSCRVAR_EXTRA_PRORATION=80,  // writable, per player. All incoming damage gets divided by this percentage. Gets reset to 100 on stage reset
    BBSCRVAR_ANIMATION_DIDNT_ADVANCE=81,  // is true during hitstop, superfreeze and on every second frame of an RC slowdown period. Actually, this returns an enum: 0 for when time flow is normal, 1 for when in hitstop, 2 for when skipping the frame due to RC slowdown, 3 for when in superfreeze
    // 82: writable, per player
    BBSCRVAR_POS_X_RAW=83,  // writable. Your current origin point's X coordinate. Origin point is the point between your feet. This position is never multiplied by your facing, neither when reading, nor when setting
    BBSCRVAR_IS_BOSS=84,  // per player. You're the boss if you're the last opponent in the Story/Arcade mode
    BBSCRVAR_IS_LOCATION_TEST=85,  // per player. This probably refers to the location test of Xrd Rev 2.1
    BBSCRVAR_STANDING_COUNTER_IS_ZERO=86,  // per player. Reports if STANDING_COUNTER is 0. Always returns false in combo trials mode, mission mode and tutorial mode
    BBSCRVAR_STANDING_COUNTER=87,  // writable, per player. Decremenets by 1 each frame, including during hitstop and superfreeze. It is called this way because it is used primarily by standing animations
    BBSCRVAR_PROJECTILE_DAMAGE_SCALE=88,  // writable. Percentage that affects damage of incoming attacks that have > 0 projectile durability/level. Projectiles that attack you set their level using setProjectileDurability beforehand
    BBSCRVAR_TRAINING_MODE_AND_NO_ONE_IN_XSTUN_OR_THROW_INVUL_FROM_XSTUN_OR_AIRBORNE_OR_ATTACKING=89,  // per player. By X-Stun I mean hitstun or blockstun
    BBSCRVAR_IS_TRAINING_MODE=90,  // global
    BBSCRVAR_ALWAYS_ZERO=91,  // this always returns 0, no matter what
    BBSCRVAR_IS_PERFORMED_RAW=92,  // this means that the move was not cancelled into from another move
    BBSCRVAR_PLAYER_SIDE=93,  // 0 for P1 effects and player, 1 for P2 effects and player
    // 95: per player
    BBSCRVAR_DISTANCE_FROM_THIS_CENTER_TO_ENEMY_CENTER_DUPLICATE=96,  // same as DISTANCE_FROM_THIS_CENTER_TO_ENEMY_CENTER
    BBSCRVAR_PITCH=97,  // writable. In 1/1000'th of a degree. So 1000 means 1 degree. Means same thing as ROTATION. 0 is unrotated and positive angle rotates you counter-clockwise when facing right, or clockwise when facing left. Pitch is the angle of rotation around the axis perpendicular to the screen
    BBSCRVAR_SCALE_X=99,  // writable. 1000 means unscaled. Scales horizontally
    BBSCRVAR_SCALE_X_GROWTH=100,  // writable. SCALE_X will increase by this amount each frame until you reset it to 0
    BBSCRVAR_EX_KIZETSU=101,  // writable, per player. Setting this to true will make it so that the next time you recover from a move you will enter "ExKizetsu" state. This value does not automatically get cleared to 0
    BBSCRVAR_EXPOINT_X=102,  // writable. These two variables are set by exPointFReset, exPointFResetObject, and can also be written to directly. They get reset to 0 on stage reset. When Venom hits a ball, for the ball this is set to the middle point between Venom and ball. POS_TYPE EX_POINT_F refers to these variables
    BBSCRVAR_EXPOINT_Y=103,  // writable. See EXPOINT_X
    BBSCRVAR_OPPONENT_X_OFFSET_TOWARDS_FACING=104,  // this is X distance to the opponent except if you're facing away from them, then it is negative
    BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL=105,  // upon0_21 has an ANIMATION_FRAME_ADVANCED event. When first registering for that event, this value gets reset to 0. It advances each frame by 1 after ANIMATION_FRAME_ADVANCED event is fired and onIdling procedure is called. It is only incremented when not in hitstop, not in superfreeze, and increments at half the rate during RC slowdown.
    BBSCRVAR_MIN_HORIZ_DIST_BETWEEN_PUSHBOXES=106,  // gets distance from your pushbox to enemy player's pushbox. If you're a projectile, it takes the projectile's pushbox
    BBSCRVAR_PLAYER_TENSION_MODE=107,  // writable, per player. Value 0 means IK_NOT_ACTIVE, 1: RED_IK_ACTIVE, 2: GOLD_IK_ACTIVE, 3: TENSION_DISABLED_UNTIL_END_OF_ROUND
    // 108: writable, per player
    BBSCRVAR_PLAYER_TENSION=109,  // writable, per player. 0 to 10000. When IK is active or tension is disabled until end of round, returns 0
    BBSCRVAR_MATCH_FINISHED=111,  // global. Is not the same as negated MATCH_RUNNING
    BBSCRVAR_HIGHER_PITCH_VOICE_TIMER=112,  // writable, per player. If you're a projectile, returns your player's timer. Decreases by 1 each frame, including frozen frames
    BBSCRVAR_CANT_BACKDASH_TIMER=113, // writable, per player. While this timer is not 0, you can't backdash. Decrements each frame
    BBSCRVAR_IS_TOUCHING_WALL_OR_SCREEN_EDGE=114,
    BBSCRVAR_CREATE_ARG_HIKITSUKI_VAL_1=115,  // writable. This can be used to read the first argument of the createArgHikitsukiVal call that was used before creating this projectile. Setting this variable is not the same as calling createArgHikitsukiVal with the same argument. The argument that you receive from createArgHikitsukiVal when you're created and the argument that you send via createArgHikitsukiVal are two separate variables. This is the argument you receive
    BBSCRVAR_CREATE_ARG_HIKITSUKI_VAL_2=116,  // writable. This can be used to read the second argument of the createArgHikitsukiVal call that was used before creating this projectile. Setting this variable is not the same as calling createArgHikitsukiVal with the same argument. The argument that you receive from createArgHikitsukiVal when you're created and the argument that you send via createArgHikitsukiVal are two separate variables. This is the argument you receive
    BBSCRVAR_LANDING_STIFF_FIELD_2=117,  // writable, per player. If the second argument of landingStiffTime had 0x2 flag, then this is equal to the first argument of landingStiffTime - the landing recovery, otherwise this is 0. This seems to be written only, but never read by the game engine
    // 119: writable, per player
    BBSCRVAR_BURST_METER=120,  // writable, per player. Capped at 15000
    // 121: only returns non-0 in MOM mode
    // 122
    BBSCRVAR_INTERROUND_VALUE_STORAGE1=123,  // writable, per player. Allows to store values between rounds
    BBSCRVAR_INTERROUND_VALUE_STORAGE2=124,  // writable, per player. Allows to store values between rounds
    BBSCRVAR_SUPERS_ARE_UNBLOCKABLE=126,  // per player. This flag changes the behavior of one super for each Sign character
    // 127
    BBSCRVAR_LAST_DEALT_COLLISION_ID=128, // this is a unique ID assigned to the last attack you connected on anything. When this ID is generated, the victim is given the same ID in their LAST_RECEIVED_COLLISION_ID
    BBSCRVAR_LAST_RECEIVED_COLLISION_ID=129, // this is a unique ID assigned to the last attack that connected on you. When this ID is generated, the attacker is given the same ID in their LAST_DEALT_COLLISION_ID
    BBSCRVAR_IS_PLAYER=130,  // false (0) for projectiles, true (1) for players
    BBSCRVAR_IS_PLAYING_CUTSCENE_ANIME=131,  // does not go to your corresponding player if accessed from effect. So effects will always read 0 here
    // 132: global
    // 133: global, per player
    BBSCRVAR_ALWAYS_ZERO3=134,  // this variable always returns 0, no matter what
    // 135: writable, per player
    // 136: global
    BBSCRVAR_INSTANT_KILLED=137,  // per player. The type of the received attack is Instant Kill
    // 138: writable, global
    BBSCRVAR_PLAYER_OR_ENEMY_ACTOR_PLAYING_CUTSCENE_ANIME=139,  // per player
    BBSCRVAR_PLAYER_ACTOR_PLAYS_CUTSCENE_ANIME=140,  // per player
    BBSCRVAR_ENEMY_ACTOR_PLAYS_CUTSCENE_ANIME=141,  // per player
    // 142: related to playing cutscene anime
    BBSCRVAR_IS_PLAYING_SOUND=143,
    // 144: global
    // 145, 146, 148, 149, 150, 151: per player
    BBSCRVAR_HELLFIRE=152,  // per player. Hellfire is a state when you have < 10% health while being combo'd or < 20% in neutral
    BBSCRVAR_LOWEST_HELD_BUTTON=153,  // per player. Possible values: 0: none, 1: PUNCH, 2: KICK, 3: SLASH, 4: HEAVY SLASH, 5: DUST, 6: TAUNT
    BBSCRVAR_IS_CPU=154,  // per player
    BBSCRVAR_IS_A_CPU_WHO_IS_NOT_PERFORMING_AN_EMOVE=155,  // per player
    BBSCRVAR_NORMALS_ENABLED=156,  // per player. Ability to perform normals is commonly regarded as the state of being "idle" or not performing any move
    // 157: writable, per player
    BBSCRVAR_HIT_BY_DUST=158,  // is hit by a dust (5D) attack
    BBSCRVAR_STYLISH=159,  // per player. Is the player using Stylish mode
    // 160
    BBSCRVAR_WAS_INPUT_USING_STYLISH=161,  // the move was performed using the Special button
    BBSCRVAR_NUMBER_OF_HITS_MAX=162,  // writable. Does not decrease with each hit taken. Specifies the maximum number of hits the projectile can take before getting destroyed (see requestDestroy). Is set using numberOfHits function or directly. When setting a value < 0, 0 is set instead.
    BBSCRVAR_NUMBER_OF_HITS_TAKEN=163,  // increments each hit taken. If destroyAssariEx is set to 1 (default is 0) or if destroyAiuchi is not set to 1 (default is 0), you get destroyed when you reach NUMBER_OF_HITS_MAX 
    BBSCRVAR_IS_BURST_SUPER=164,  // per player
    BBSCRVAR_DUST_GATLING_TIMER=165, // writable, per player. During this timer, any non-follow-up, non-super, non-IK move can be cancelled into from any normal. Decrements by 1 each frame
    BBSCRVAR_HITSTUN=166,  // writable, per player. Returns the current remaining number of frames of hitstun of your player
    BBSCRVAR_TUMBLE_DURATION=167, // writable, per player. The duration of the received tumble. Does not decrement over time. Instead, CmnActKorogari (tumble animation) has an internal counter that counts up to this value.
    BBSCRVAR_STYLISH_DAMAGE_DIVISOR=168,  // writable, global. All incoming damage is divided by this percentage for players who are using the Stylish mode
    BBSCRVAR_STYLISH_TENSION_MODIFIER=169,  // writable, global. A percentage. All incoming tension is multiplied by it for players who are using the Stylish mode
    BBSCRVAR_STYLISH_BURST_GAIN_MODIFIER=170,  // writable, global. A percentage. All incoming burst gauge values are multiplied by it for players who are using the Stylish mode
    BBSCRVAR_DOKI_DOKI_STOP_HASSEI_ENABLED=172,  // writable, global. Specify 0 or 1. If 1, Dramatic Finale is allowed to happen. If 0, it can never happen. This variable is global. The default value is 1.
    BBSCRVAR_DOKI_DOKI_STOP_HASSEI_PRE_TIME=173,  // writable, global. Specify the amount of time, in frames, that Dramatic Finale happens before the killing blow attacks become active. This variable is global. The default value is 6.
    BBSCRVAR_DOKI_DOKI_X_DIST_MIN=174,  // writable, global. The minimum distance between players' X position to trigger Dramatic Finale. This is just one of the checks that must be satisfied to trigger Dramatic Finale. This variable is global. Default value is 100000, which is the distance at which players' pushboxes are touching
    BBSCRVAR_DOKI_DOKI_X_DIST_MAX=175,  // writable, global. The maximum distance between players' X position to trigger Dramatic Finale. This is just one of the checks that must be satisfied to trigger Dramatic Finale. This variable is global. Default value is 500000
    BBSCRVAR_DOKI_DOKI_MAX_DIST_Y=176,  // writable, global. The maximum distance between players' Y position to trigger Dramatic Finale. This is just one of the checks that must be satisfied to trigger Dramatic Finale. This variable is global. Default value is 360000
    BBSCRVAR_STANDARD_DOKI_DOKI_STOP_HASSEI=177,  // writable, global. Default value is 7. This provides the standard value for moves that have not set a timing using the dokiDokiStopHassei function. This variable is global.
    // 178: writable, global
    BBSCRVAR_ALWAYS_MINUS_ONE=197,  // this always returns -1, no matter what
    // 198: writable, global
    BBSCRVAR_DOKI_DOKI_COOLDOWN_TIMER=199,  // writable, global. A timer that is stored in the game engine, which decrements by 1 each frame, including superfreeze. While it is above 0, Dramatic Finale cannot happen
    BBSCRVAR_TIME_SINCE_CREATION=200,  // starts from 0 and counts time, including frozen frames, since creation, and never gets reset until the object is destroyed
    // 201, 202, 203, 204: writable, per player. These variables can be used in move conditions
    BBSCRVAR_ALWAYS_ZERO_205=205,  // this always returns 0, no matter what
    BBSCRVAR_ALWAYS_ZERO_206=206,  // this always returns 0, no matter what
    BBSCRVAR_ALWAYS_ZERO_207=207,  // this always returns 0, no matter what
    BBSCRVAR_ALWAYS_ZERO_208=208,  // this always returns 0, no matter what
    BBSCRVAR_OPPONENT_ALREADY_HIT_BY_NEOBLITZ=209,  // per player. Neo blitz means blitz shield attack
    BBSCRVAR_DUST_PRORATION1=210,  // writable, per player. This proration percentage is unconditionally applied to all incoming damage. Set this on the one who is going to take the damage
    BBSCRVAR_DUST_PRORATION2=211,  // writable, per player. This proration percentage is unconditionally applied to all incoming damage. Set this on the one who is going to take the damage
    BBSCRVAR_IS_STYLISH_OR_CPU=212,  // per player
    BBSCRVAR_ENABLE_NORMALS=213,  // per player. Same as NORMALS_ENABLED
    BBSCRVAR_OPPONENT_IN_HITSTUN=214,  // per player. Includes the frame when hitstun is entered into. Returns 0 or 1
    BBSCRVAR_IS_DARK_COLOR=215,  // per player. This is something that can be set in Arcade (Story) mode per opponent, but no one has dark color set to true. When this is true, if your (assuming you're the AI opponent) color is the same as that of the human player, and both use the same character, the you (the AI) don't change your color to mirror color. When this is false, if both players use the same character and color, the AI (you) are changed to the mirror (dark) color. This value does not by itself reflect whether you (the AI) currently have the dark color. It merely means whether the check for resolving a color collision with dark color was enabled for the AI opponent. When this value is accessed by human player or their effects, it should always return 0, because there is no way to set IS_DARK_COLOR to true for the human player in arcade mode. This should be always false even for the AI opponent, because there are no arcade missions that have set this value to true.
    BBSCRVAR_HIT_BY_NORMAL_THROW=216, // checks whether you were hit by an attack that has either kuuchuutuujounageTsukami (normal airthrow) or chijoutuujounageTsukami (normal ground throw) flags. These flags are only ever set on normal ground and air throws and disappear as soon as the non-throw portion of the H/OS move starts in case the throw whiffed. Meaning that if you got hit by an attack that was 4H/6H or a throw OS, but the attack itself was not a throw, then HIT_BY_NORMAL_THROW would be false on this hit. But if you get hit by the throw portion of that attack, then HIT_BY_NORMAL_THROW would return true.
    BBSCRVAR_HIT_BY_OVERDRIVE=217,
    // 218: global
    // 219, 220, 221, 222, 223, 224, 225: writable, global
    BBSCRVAR_ALPHA=226,  // writable. This is the alpha (transparency) of the color. From 0 to 255. Setting this to 0 makes you fully transparent
    // 227: global
    // 228: writable, per player
    BBSCRVAR_SLOW_SPEED_PERCENTAGE=229,  // writable, per player. Percentage that multiplies your movement speed, making it slower if < 100 or faster if > 100
    // 230: global
    BBSCRVAR_IS_TRIAL_COMPLETED=231,  // global, per trial
    BBSCRVAR_TRIAL_BEST_TIME=232,  // writable, global, per trial
    BBSCRVAR_TRIAL_TIME=233,  // writable, global
    BBSCRVAR_MOM_INFINITE_DRAGON_INSTALL=234,  // apparently, some item in MOM causes Sol to enter DI indefinitely
    BBSCRVAR_COSTUME=235,  // used by Elphelt to determine selected outfit
    BBSCRVAR_ARMORED_HITS_ON_THIS_FRAME=236,  // per player
};

enum BBScrPosType {
    BBSCRPOSTYPE_ORIGIN=100,  // the origin point that is between your legs
    BBSCRPOSTYPE_ATTACK=101,
    BBSCRPOSTYPE_DAMAGE=102,  // this is last point of the hit which is at the center of the intersection between an attacker's hitboxes and a defender's pushboxes
    BBSCRPOSTYPE_CENTER=103,  // center of your body depends on your stance: crouching and lying poses body centers are lower than standing
    BBSCRPOSTYPE_FLOOR_CENTER=104,  // same as LAND
    BBSCRPOSTYPE_BODY_RANDOM=105,
    BBSCRPOSTYPE_EX_RECT_RANDOM=106,
    BBSCRPOSTYPE_AURA=107,
    BBSCRPOSTYPE_PRIVATE_0=108,
    BBSCRPOSTYPE_PRIVATE_1=109,
    BBSCRPOSTYPE_PRIVATE_2=110,
    BBSCRPOSTYPE_PRIVATE_3=111,
    BBSCRPOSTYPE_PAST_HALF=112,  // the middle position of the origin point between past frame's X;Y and this frame's X;Y
    BBSCRPOSTYPE_GTMP=113,  // returns a point made up of GTMP_X and GTMP_Y
    BBSCRPOSTYPE_HEAD=114,
    BBSCRPOSTYPE_NECK=115,
    BBSCRPOSTYPE_ABDOMINAL=116,
    BBSCRPOSTYPE_R_LEG=117,
    BBSCRPOSTYPE_L_LEG=118,
    BBSCRPOSTYPE_HEART=119,
    BBSCRPOSTYPE_ENEMY_LAND=120,
    BBSCRPOSTYPE_FLOOR_ALIAS=121,
    BBSCRPOSTYPE_ASHIMOTO_RANDOM=122,
    BBSCRPOSTYPE_LAND=123,  // land is your current X position with the Y position of floor
    BBSCRPOSTYPE_FRONT_05_BODY=124,
    BBSCRPOSTYPE_FRONT_1_BODY=125,
    BBSCRPOSTYPE_FRONT_2_BODY=126,
    BBSCRPOSTYPE_BACK_05_BODY=127,
    BBSCRPOSTYPE_BACK_1_BODY=128,
    BBSCRPOSTYPE_BACK_2_BODY=129,
    BBSCRPOSTYPE_EX_POINT_F=130,  // refers to EXPOINT_X, EXPOINT_Y variables
    BBSCRPOSTYPE_WORLD_ZERO=131,  // the 0;0 point of the arena, which is its middle on the floor level
    BBSCRPOSTYPE_ENEMY_CENTER=132,
    BBSCRPOSTYPE_ENEMY_BODY_RANDOM=133,
    BBSCRPOSTYPE_ENEMY_EX_RECT_RANDOM=134,
    BBSCRPOSTYPE_ENEMY_AURA=135,
    BBSCRPOSTYPE_ENEMY_HEAD=136,
    BBSCRPOSTYPE_ENEMY_NECK=137,
    BBSCRPOSTYPE_ENEMY_ABDOMINAL=138,
    BBSCRPOSTYPE_ENEMY_R_LEG=139,
    BBSCRPOSTYPE_ENEMY_L_LEG=140,
    BBSCRPOSTYPE_ENEMY_HEART=141,
    BBSCRPOSTYPE_ENEMY_EX_POINT_F=142,
    BBSCRPOSTYPE_EX_POINT_PG=143,
    BBSCRPOSTYPE_BODY_RANDOM_3D=144,
    BBSCRPOSTYPE_EX_POINT_RANDOM=145,
    BBSCRPOSTYPE_BIKKURI=146,
    BBSCRPOSTYPE_CHEST_3D=147,
    BBSCRPOSTYPE_LEG_CENTER=148,
    BBSCRPOSTYPE_HEAD_3D=149,
    BBSCRPOSTYPE_PELVIS_3D=150,
    BBSCRPOSTYPE_BODY_RANDOM_3D_NO_HEAD=151,
    BBSCRPOSTYPE_SHOULDER_L_3D=152,
    BBSCRPOSTYPE_SHOULDER_R_3D=153,
    BBSCRPOSTYPE_ELBOW_L_3D=154,
    BBSCRPOSTYPE_ELBOW_R_3D=155,
    BBSCRPOSTYPE_HAND_L_3D=156,
    BBSCRPOSTYPE_HAND_R_3D=157,
    BBSCRPOSTYPE_CALF_L_3D=158,
    BBSCRPOSTYPE_CALF_R_3D=159,
    BBSCRPOSTYPE_FOOT_L_3D=160,
    BBSCRPOSTYPE_FOOT_R_3D=161,
    BBSCRPOSTYPE_AFRO_3D=162,
    BBSCRPOSTYPE_FRONT_3_BODY=163,
    BBSCRPOSTYPE_BACK_3_BODY=164,
    BBSCRPOSTYPE_SCREEN_CENTER=165,
};

struct AccessedValue {
	BBScrTag tag;
	union {
		BBScrVariable var;
		int value;
	};
	inline explicit AccessedValue(const void* ptr) : tag(*(BBScrTag*)ptr), value(*((int*)ptr + 1)) { }
	inline explicit AccessedValue(BBScrTag tag, int value) : tag(tag), value(value) { }
	inline explicit AccessedValue(BBScrVariable var) : tag(BBSCRTAG_VARIABLE), value(var) { }
	inline bool operator==(const AccessedValue& other) const noexcept {
		return tag == other.tag
			&& value == other.value;
	}
	inline bool operator!=(const AccessedValue& other) const noexcept {
		return tag != other.tag
			|| value != other.value;
	}
	inline bool operator==(BBScrVariable var) const noexcept {
		return tag == BBSCRTAG_VARIABLE
			&& value == var;
	}
	inline bool operator!=(BBScrVariable var) const noexcept {
		return !(*this == var);
	}
	inline bool operator==(BBScrTag tag) const noexcept {
		return this->tag == tag;
	}
	inline bool operator!=(BBScrTag tag) const noexcept {
		return this->tag != tag;
	}
	inline bool operator!() const noexcept {
		return !(bool)*this;
	}
	inline operator bool() const noexcept {
		if (tag == BBSCRTAG_VALUE && value == 0) return false;
		return true;
	}
};

#define MEM(x) (BBScrVariable)x

enum InstrType {
	// these are from bbscript database: https://github.com/super-continent/bbscript
	instr_endState = 1,
	instr_sprite = 2,
	instr_spriteEnd = 3,
	instr_if = 4,
	instr_ifOperation = 6,
	instr_else = 9,
	instr_endElse = 10,
	instr_setMarker = 11,
	instr_goToMarker = 12,
	instr_gotoLabelRequests = 14,
	instr_callSubroutine = 17,
	instr_exitState = 18,
	instr_deactivateObj = 19,
	instr_upon = 21,
	instr_endUpon = 22,
	instr_clearUpon = 23,
	instr_overrideSpriteLengthIf = 26,
	instr_jumpToState = 27,
	instr_deactivateObjectByName = 32,
	instr_runOnObject = 41,
	instr_storeValue = 46,
	instr_checkInput = 48,
	instr_checkMoveCondition = 49,
	instr_modifyVar = 54,
	instr_calcDistance = 60,
	instr_hit = 234,
	instr_recoveryState = 235,
	instr_ignoreDeactivate = 298,
	instr_createObjectWithArg = 445,
	instr_createObject = 446,
	instr_createParticleWithArg = 449,
	instr_linkParticle = 450,
	instr_setLinkObjectDestroyOnStateChange = 457,
	instr_attackLevel = 711,
	instr_hitAirPushbackX = 754,
	instr_groundHitEffect = 882,
	instr_wallstickDuration = 898,
	instr_blockstunAmount = 1023,
	instr_stunValue = 1096,
	instr_hitPushbackX = 1102,
	instr_deleteMoveForceDisableFlag = 1603,
	instr_whiffCancelOptionBufferTime = 1630,
	instr_sendSignal = 1766,
	instr_sendSignalToAction = 1771,
	instr_linkParticleWithArg2 = 1923,
	instr_exPointFReset = 2161,
	instr_requestDestroy = 2176,
	instr_numberOfHits = 2179,
	instr_timeSlow = 2201,
	instr_createArgHikitsukiVal = 2247,
	instr_setHitstop = 2263,
};

struct BBScrInstr_if {
	InstrType type;
	AccessedValue operand;
};

struct BBScrInstr_ifOperation {
	InstrType type;
	BBScrOperation op;
	AccessedValue left;
	AccessedValue right;
};

struct BBScrInstr_calcDistance {
	InstrType type;
	EntityReferenceType fromEntity;
	BBScrPosType fromPos;
	EntityReferenceType toEntity;
	BBScrPosType toPos;
};

struct BBScrInstr_exPointFReset {
	InstrType type;
	BBScrPosType pos;
	int x;
	int y;
};

struct BBScrInstr_sprite {
	InstrType type;
	char name[32];
	int duration;
};

struct BBScrInstr_sendSignalToAction {
	InstrType type;
	char name[32];
	BBScrEvent signal;
};

struct BBScrInstr_overrideSpriteLengthIf {
	InstrType type;
	int duration;
	AccessedValue var;
};

typedef struct BBScrInstr_upon {
	InstrType type;
	BBScrEvent event;
} BBScrInstr_clearUpon;

typedef struct BBScrInstr_setMarker {
	InstrType type;
	char name[32];
} BBScrInstr_callSubroutine, BBScrInstr_jumpToState;

struct BBScrInstr_createObjectWithArg {
	InstrType type;
	char name[32];
	BBScrPosType pos;
};

struct BBScrInstr_setLinkObjectDestroyOnStateChange {
	InstrType type;
	EntityReferenceType entity;
};

struct BBScrInstr_sendSignal {
	InstrType type;
	EntityReferenceType entity;
	BBScrEvent event;
};

struct BBScrInstr_storeValue {
	InstrType type;
	AccessedValue dest;
	AccessedValue src;
};

struct BBScrInstr_createObject {
	InstrType type;
	char name[32];
	BBScrPosType pos;
};

struct BBScrInstr_deactivateObjectByName {
	InstrType type;
	char name[32];
};

struct BBScrInstr_gotoLabelRequests {
	InstrType type;
	char name[32];
};

struct BBScrInstr_beginState {
	InstrType type;
	char name[32];
};

struct BBScrInstr_modifyVar {
	InstrType type;
	BBScrOperation op;
	AccessedValue left;
	AccessedValue right;
};

struct BBScrInstr_requestDestroy {
	InstrType type;
	EntityReferenceType entity;
};

struct BBScrInstr_deactivateObj {
	InstrType type;
	EntityReferenceType entity;
};

struct BBScrInstr_stunValue {
	InstrType type;
	int amount;
};

struct BBScrInstr_wallstickDuration {
	InstrType type;
	int amount;
};

struct BBScrInstr_blockstunAmount {
	InstrType type;
	int amount;
};

struct BBScrInstr_hitPushbackX {
	InstrType type;
	int amount;
};

struct BBScrInstr_attackLevel {
	InstrType type;
	int amount;
};

#define asInstr(instr, bbscrInstrName) ((BBScrInstr_##bbscrInstrName*)instr)

struct CmnActHashtable {
	unsigned short maximumBucketLoad;
	unsigned short currentSize;
	unsigned short hashMap[800];
	unsigned short next[200];
	char strings[200][32];
};

enum HitEffect {
	HIT_EFFECT_GROUND_NORMAL = 0,  // does either crouching hitstun, high standing or low standing hitstun, depending on whether the opponent is crouching and how high the attack hit them
	HIT_EFFECT_AIR_NORMAL = 1,  // exact same as AIR_FACE_UP
	HIT_EFFECT_CRUMPLE = 2,  // causes Hizakuzure animation. This later leads to face down wakeup
	HIT_EFFECT_FORCE_CROUCH = 3,  // crouching hitstun
	HIT_EFFECT_FORCE_STAND_HIGH = 4,  // high standing hitstun
	HIT_EFFECT_FORCE_STAND_LOW = 5,  // low standing hitstun
	HIT_EFFECT_BLITZ_REJECT_STAND = 6,  // causes standing Hajikare animation
	HIT_EFFECT_BLITZ_REJECT_CROUCH = 7,  // causes crouching Hajikare animation
	HIT_EFFECT_AIR_FACE_UP = 8,  // if the speed applied to the opponent is directed downward and they're grounded, they immediately get knocked down onto their back. Otherwise launches into air such that you land face up
	HIT_EFFECT_AIR_LAUNCH = 9,  // if the speed applied to the opponent is directed downward and they're grounded, they immediately get knocked down onto their face. Launches into air vertically (it's a different animation). You will end up face down
	HIT_EFFECT_AIR_FACE_DOWN = 10,  // launches into air such that you land face down
	HIT_EFFECT_AIR_STRONG = 11, // causes Blowoff animation. Leads to face up knockdown
	HIT_EFFECT_KIRIMOMI = 12,  // the animation you see when you get hit by 5D. Leads to vertical fall which leads to face down knockdown
	HIT_EFFECT_KIRIMOMI_B = 13,  // same as KIRIMOMI
	HIT_EFFECT_BLITZ_REJECT_AIR = 14,  // causes air Hajikare animation
	HIT_EFFECT_STAGGER = 15,  // causes Jitabata animation during which you must mash to get out and a light blue bar with a button graphic is shown on top of you
	HIT_EFFECT_EX_DAMAGE_BALL_MOF = 16,  // same as EX_DAMAGE
	HIT_EFFECT_EX_DAMAGE = 17,  // EX_DAMAGE is getting hit by Elphelt 236236K or Venom throw. I think it just launches you relatively high and transitions to CmnActB/VDownUpper
	HIT_EFFECT_EX_DAMAGE_LAND = 18,
	HIT_EFFECT_BLITZ_REJECT_AUTODECIDE_STANDCROUCHAIR = 19,
	HIT_EFFECT_BLITZ_REJECT_AUTODECIDE_STANDCROUCHAIR2 = 20,  // same as BLITZ_REJECT_AUTODECIDE_STANDCROUCHAIR
	HIT_EFFECT_NORMAL_AUTODECIDE_HIGHERLOWER = 21,  // standing hitstun, high/low version depends on how high the attack connected
	HIT_EFFECT_THROW_CLASH_AUTODECIDE_CROUCHSTAND = 22,  // same as BLITZ_REJECT_AUTODECIDE_STANDCROUCHAIR but shorter (30f) rejection period instead of 60f
	HIT_EFFECT_THROW_CLASH_STAND = 23,  // same as BLITZ_REJECT_STAND, but shorter (30f) rejection period instead of 60f
	HIT_EFFECT_THROW_CLASH_CROUCH = 24,  // same as BLITZ_REJECT_CROUCH, but shorter (30f) rejection period instead of 60f
	HIT_EFFECT_THROW_CLASH_AIR = 25,  // same as BLITZ_REJECT_AIR, but shorter (30f) rejection period instead of 60f
	HIT_EFFECT_AIR_SOMERSAULT = 26,  // causes ZSpin animation, which leads to face down knockdown
	HIT_EFFECT_UNSPECIFIED = INT_MAX  // default value. Translates to GROUND_NORMAL for groundHitEffect and AIR_NORMAL for airHitEffect
};

// Everything gets reset to INT_MAX
struct InflictedParameters {
	int impulseX;  // 0x0
	int impulseY;  // 0x4
	int gravity;  // 0x8
	int wallBounceXVelocityPercent;  // 0xc
	int groundBounceYVelocityPercent;  // 0x10
	int hitstunIfItWasGrounded;  // 0x14
	int untechableTime;  // 0x18
	int wallbounceCount;  // 0x1c
	int groundBounceCount;  // 0x20
	int knockdownDuration;  // 0x24
	HitEffect groundHitEffect;  // 0x28
	HitEffect airHitEffect;  // 0x2c
	int wallstickDuration;  // 0x30
	int wallBounceInCornerOnly;  // 0x34  // used as a BOOL. But yes, default value is still INT_MAX
	int tumbleDuration;  // 0x38  // received, maximum, doesn't decrease over time
	int wallBounceWaitTime;  // 0x3c
	int tumbleCount;  // 0x40
};

struct FName {
	int low;
	int high;
	static FName nullFName;
	char* print(char* buf, size_t bufSize) const;
	template<size_t size>
	inline char* print(char(&buf)[size]) const { return print(buf, size); }
	inline bool operator==(const FName& other) const { return low == other.low && high == other.high; }
	inline bool operator!=(const FName& other) const { return low != other.low || high != other.high; }
};

inline DWORD hash(const FName* fname) { return (DWORD)fname->low; }

#pragma pack(push, 4)
class UObject {
public:
	void* vtable;
	UObject* HashNext;
	unsigned long long ObjectFlags;
	UObject* HashOuterNext;
	void* StateFrame;
	void* _Linker;
	int* _LinkerIndex;
	int Index;
	int NetIndex;
	UObject* Outer;
	FName Name;
	void* Class;
	UObject* ObjectArchetype;
};
#pragma pack(pop)

class FString {
public:
	wchar_t* Data;
	int ArrayNum;
	int ArrayMax;
};

struct FUntypedBulkData {
	void* vtable;
	DWORD BulkDataFlags;
	int ElementCount;
	int BulkDataOffsetInFile;
	int BulkDataSizeOnDisk;
	DWORD SavedBulkDataFlags;
	int SavedElementCount;
	int SavedBulkDataOffsetInFile;
	int SavedBulkDataSizeOnDisk;
	void* BulkData;
	DWORD LockStatus;
	void* AttachedAr;
	BOOL bShouldFreeOnEmpty;
};

template<typename T>
class REDAssetBase : public UObject {
public:
	T* TopData;
	DWORD DataSize;
	FUntypedBulkData BulkDataLE;
	FUntypedBulkData BulkDataBE;
	FString SourceFile;
};

struct FPACLookupElement0x30 {
	char spriteName[32];
	DWORD index;
	DWORD offset;
	DWORD size;
	DWORD hash;
};

struct FPACLookupTable0x30 {
	FPACLookupElement0x30 elements[1];
	void insertAt(DWORD index, DWORD count, const FPACLookupElement0x30& newElement);
};

struct FPACLookupElement0x10 {
	DWORD hash;
	char unknown[12];
};

struct FPACLookupTable0x10 {
	DWORD unknown[3];
	FPACLookupElement0x10 elements[1];
};

struct FPACLookupElement0x50 {
	char spriteName[32];
	char unknown[0x20] { 0 };
	DWORD index;
	DWORD offset;
	DWORD size;
	DWORD hash;
};

struct FPACLookupTable0x50 {
	FPACLookupElement0x50 elements[1];
	void insertAt(DWORD index, DWORD count, const FPACLookupElement0x50& newElement);
};

union FPACLookupUnion {
	FPACLookupTable0x10 table0x10;
	FPACLookupTable0x30 table0x30;
	FPACLookupTable0x50 table0x50;
};

struct FPAC {
	char magic[4];  // "FPAC". Offset 0x0
	DWORD headerSize;  // size of FPAC data before lookup entries and all lookup entries. Offset 0x4
	DWORD rawSize;  // the size of the whole FPAC. Offset 0x8
	DWORD count;  // count of lookup entries. Offset 0xc
	DWORD flags;  // offset 0x10
	inline bool useHash() const { return (flags & 0x80000000); }
	inline bool size0x50() const { return (flags & 0x20000000); }
	// bedman, answer and raven have 0x20000000 flag and 0x80000000 and 0x40000000 flags, everyone else only has 0x80000000 and 0x40000000
	inline bool flag2() const { return (flags & 0x2); }  // not encountered in Xrd. Good. Less work
	DWORD sizeOfSingleElement;  // for when !useHash(). Offset 0x14
	char unknown[8];  // offset 0x18
	FPACLookupUnion data;  // offset 0x20
	// the callback receives each sprite name and returns whether to continue enumeration. True to continue, false to stop
	void enumNames(bool (*callback)(char* name, BYTE* jonbin));
	static int calcJonbSize(BYTE* dataPtr);
	static const int size1 = 0x1a * 2;
	static const int size2 = 0x28 * 2;
	static const int size3 = 0x9a * 2;
	inline DWORD elementSize() const { return size0x50() ? 0x50 : 0x30; }
	DWORD findInsertionIndex(DWORD hash);
	DWORD sizeOfJonbinAtOffset(DWORD offset, DWORD* offsetPtrOffset);
	BYTE* findLookupEntry(DWORD hash);
	BYTE* findLookupEntry(const char* str);
	BYTE* lookupEnd() const;
};

// If both Players are the same character, they share the same REDAssetCollision
class REDAssetCollision : public REDAssetBase<FPAC> {
public:
};

template<typename T>
struct TArray {
	T* Data;
	int ArrayNum;
	int ArrayMax;
};

template<size_t size, typename T>
struct TInlineAllocator {
	T InlineData[size];
	T* Data;
};

struct TBitArray {
	DWORD InlineData[4];
	DWORD* Data;
	int NumBits;
	int MaxBits;
};

template<typename T>
struct TSparseArray {
	TArray<T> Data;
	TBitArray AllocationFlags;
	int FirstFreeIndex;
	int NumFreeIndices;
};

template<typename T>
struct TSet {
	struct FElement {
		T Value;
		int HashNextId;
		int HashIndex;
	};
	TSparseArray<TSet<T>::FElement> Elements;
	TInlineAllocator<1,int> Hash;
	int HashSize;
};

template<typename Key, typename Value>
struct TMap {
	struct FPair {
		Key key;
		Value value;
	};
	TSet<TMap<Key,Value>::FPair> Pairs;
	int find(Key* key) const;
};

struct UAnimSequence : public UObject {
	FName SequenceName;
	// there's more data
	float SequenceLength() const { return *(float*)((BYTE*)this + 0x68); }
	float RateScale() const { return *(float*)((BYTE*)this + 0x70); }
};

struct UAnimSet {
	char knowndata[0x4c];
	TArray<UAnimSequence*> Sequences;
	TMap<FName,int> SequenceCache;  // UE3 contains code for when the cache is stale that rebuilds it. Do not trust this map, screw it
	// there's more data
	UAnimSequence* find(FName name) const;
};

struct USkeletalMeshComponent {
	char knowndefinitelyknown[0x1e4];
	BYTE* SkelMesh;
	char thistoo[0xc4];
	TArray<UAnimSet*> AnimSets;  // iterate from the end to find UAnimSequence by name. May contain null pointers.
	UAnimSequence* find(FName name) const;
};

struct UAnimNodeSequence {
	char iknowwhatshere[0x5c];  // but we don't need it
	USkeletalMeshComponent* SkelComponent;
	char other[0xb0];
	FName AnimSeqName;
	char toolazytofillin[0x14];
	UAnimSequence* AnimSeq;
	char rest[0x40];  // exact, verified. I know what's here, I'm just not filling it in because
	inline UAnimSequence* find(FName name) const { return SkelComponent->find(name); }
};

struct REDAnimNodeSequence : public UAnimNodeSequence {
	DWORD StepPlay:1;
	DWORD AnimeEnd:1;
	DWORD AnimeUpdate:1;
	DWORD FirstTime:1;
	DWORD bStepAnimeSectionLoop:1;
	DWORD bNonStepAnimUseDeltaSeconds:1;
	int CurrentFrame;
	int FrameMax;
	int StepFrame;
	float SectionLoopBeginTime;
	class REDPawn* Pawn;
};

struct MeshControl {
	char iknowwhatisherebutwontbother[0x58];
	REDAnimNodeSequence* AnimSeq;
	char rest[0x14];
	inline UAnimSequence* find(FName name) const { return AnimSeq->find(name); }
};

class REDPawn {
public:
	inline char* p() { return (char*)this; }
	inline int MeshControlNum() { return *(int*)(p() + 0x1a84); }
	inline MeshControl(&MeshControls())[50] { return *(MeshControl(*)[50])(p() + 0x4a4); }
	REDAnimNodeSequence* getFirstAnimSeq();
	int getMaxFrameOfAnimSequence(FName name);
};

class REDPawn_Player : public REDPawn {
public:
	// If both Players are the same character, they share the same REDAssetCollision
	REDAssetCollision* Collision() { return *(REDAssetCollision**)(p() + 0x4a84); }
};

struct HitboxHolder {
	void* vtable;
	BYTE* jonbinPtr;  // points to the start of JONB (jonbin) data
	BYTE* ptrLookup;  // only if not flag2()
	BYTE* ptrRawAfterShort1;  // points to after hitbox counts + short1 * size1
	Hitbox* data[17];  // use HitboxType enum
	int short2;  // short read from an FPAC
	int count[17];  // use HitboxType enum
	char* names[7];  // only the first of these matters. Must contain a '_'. The part before that is the animation sequence name. Part after - animation frame.
	char unknown[4];
	int nameCount;
	int xShift;  // these can't be set by the bbscript sprite instruction
	int yShift;
	Hitbox* hitboxesStart() const;
	static BYTE numTypes(BYTE* jonbinPtr);
	static Hitbox* hitboxesStart(BYTE* jonbinPtr);
	static short* hitboxCounts(BYTE* jonbinPtr);
	int hitboxCount() const;
	void parse(BYTE* jonbinPtr);
};

class Entity
{
public:
	inline Entity() {}
	inline Entity(void* ent) { this->ent = (char*)ent; }
	char* ent = nullptr;
	
	inline bool isActive() const { return *(DWORD*)(ent + 0xc) == 1; }
	inline bool isFinalized() const { return *(DWORD*)(ent + 0xc) == 4; }  // both 0 and 4 mean entity is deleted. 2 is NEED_SET_TO_IS_ACTIVE_AND_CONSIDER_ACTIVE_NOW. 3 is NEED_CALL_FINALIZE
	// Active means attack frames are coming
	inline bool isActiveFrames() const {
		return (*(DWORD*)(ent + 0x23C) & 0x100) != 0  // This signals that attack's hitboxes should not be ignored. Can happen before hitboxes come out
			&& (*(DWORD*)(ent + 0x234) & 0x40000000) == 0  // This signals that attack's hitboxes should be ignored.
		                                                   // Can be simultaneous with 0x100 flag in 0x23C - recovery takes priority
		                                                   // Some moves don't have this flag during their recovery
			&& dealtAttack()->type > 0;  // this check is in the hit detection check for being able to land any kind of attack, be it player or projectile
	}
	inline bool hasActiveFlag() const { return (*(DWORD*)(ent + 0x23C) & 0x100) != 0; }
	
	// 0 for P1, 1 for P2
	inline char team() const { return *(char*)(ent + 0x40); }
	
	inline bool isPawn() const { return *(bool*)(ent + 0x10); }
	inline CharacterType characterType() const { return (CharacterType)*(char*)(ent + 0x44); }
	
	inline int& x() const { return *(int*)(ent + 0x24c); }
	inline int& y() const { return *(int*)(ent + 0x250); }
	// This function is for hitbox calculation only. To obtain raw x position, use x()
	int posX() const;
	// This function is for hitbox calculation only. To obtain raw y position, use y()
	int posY() const;
	
	inline BOOL& isFacingLeft() const { return *(BOOL*)(ent + 0x248); }  // the graphical facing
	inline BOOL& inputsFacingLeft() const { return *(BOOL*)(ent + 0x4d38); }  // the facing for input motions interpreting

	bool isGettingThrown() const;
	
	void pushboxDimensions(int* left, int* top, int* right, int* bottom) const;
	
	inline bool performingThrow() const { return (*(DWORD*)(ent + 0x23c) & 0x1000) != 0; }  // this flag appears when starting a throw or throw-like move and stays until first hit connects
	inline unsigned int currentAnimDuration() const { return *(const unsigned int*)(ent + 0x130); }  // number of frames since the start of animation. Includes only non-frozen, non-hitstop and non-RC frozen frames (so only actually played sprite frames). Resets to 1 at the start of each animation
	inline unsigned int animFrameStepCounter() const { return *(const unsigned int*)(ent + 0x13c); }  // number of FRAMESTEP_1 events since the start of animation. Includes frozen frames. Resets to 1 at the start of each animation.
	inline const char* animationName() const { return (const char*)(ent + 0x2444); }
	inline CmnActIndex cmnActIndex() const { return *(CmnActIndex*)(ent + 0xa01c); }
	inline bool naguriNagurareru() const { return (*(DWORD*)(ent + 0x120) & 0x100) != 0; }
	inline bool isDisableThrow() const { return (*(DWORD*)(ent + 0x120) & 0x400) != 0; }
	inline bool guardBreakInitialProrationApplied() const { return (*(DWORD*)(ent + 0x120) & 0x1000) != 0; }
	inline bool servant() const { return (*(DWORD*)(ent + 0x120) & 0x3800000) != 0; }  // 0x800000 servant A, 0x1000000 servant B, 0x2000000 servant C
	inline bool servantA() const { return (*(DWORD*)(ent + 0x120) & 0x800000) != 0; }
	inline bool servantB() const { return (*(DWORD*)(ent + 0x120) & 0x1000000) != 0; }
	inline bool servantC() const { return (*(DWORD*)(ent + 0x120) & 0x2000000) != 0; }
	inline bool ghost() const { return (*(DWORD*)(ent + 0x120) & 0x400000) != 0; }
	inline int hitstop() const { return *(int*)(ent + 0x1ac); }
	inline int clashHitstop() const { return *(int*)(ent + 0x1b0); }
	inline bool needSetHitstop() const { return *(DWORD*)(ent + 0x1b8) != 0; }
	inline int startingHitstop() const { return *(int*)(ent + 0x1b4); }
	inline int hitstun() const { return *(int*)(ent + 0x9808); }  // this value is not used by some hitstun animations like Jitabata or CmnActBDownLoop for example and may contain garbage value during those
	inline int blockstun() const { return *(int*)(ent + 0x4d54); }
	inline int tension() const { return *(int*)(ent + 0x2d134); }  // meter
	inline int tensionPulse() const { return *(int*)(ent + 0x2d128); }  // affects how fast you gain tension
	inline int& tensionPulse() { return *(int*)(ent + 0x2d128); }  // affects how fast you gain tension
	inline int negativePenaltyTimer() const { return *(int*)(ent + 0x2d12c); }  // starts after you reach negative penalty
	inline int tensionPulsePenalty() const { return *(int*)(ent + 0x2d140); }  // reduces tension pulse and increases negative penalty
	inline int negativePenalty() const { return *(int*)(ent + 0x2d144); }  // progress towards negative penalty
	inline int cornerPenalty() const { return *(int*)(ent + 0x2d14c); }  // penalty for touching the wall
	inline int stunmashCeiling() const { return *(int*)(ent + 0x24e04); }
	inline int risc() const { return *(DWORD*)(ent + 0x24e30); }
	inline int riscResidual() const { return *(DWORD*)(ent + 0x24e34); }  // the value that UI shows in the dark red part of the RISC gauge
	inline int hp() const { return *(int*)(ent + 0x9cc); }
	inline int maxHp() const { return *(int*)(ent + 0x9d0); }
	inline int defenseModifier() const { return *(int*)(ent + 0x9874); }  // dmg = dmg * (defenseModifier + 0x100) / 0x100
	inline int gutsRating() const { return *(int*)(ent + 0x9878); }
	inline int proration() const { return *(int*)(ent + 0x9f58); }
	inline int stun() const { return *(int*)(ent + 0x9fc4); }
	inline int stunThreshold() const { return *(int*)(ent + 0x9fc8); }
	inline int weight() const { return *(int*)(ent + 0x9880); }
	// some of these are really hard to describe without comments that super continent's (Pangaea's) bbscript database doesn't have.
	// This is set when hitting someone and reset on recovery
	inline bool attackCollidedSoCanCancelNow() const { return (*(DWORD*)(ent + 0x23c) & 0x20) != 0; }
	inline bool attackCollidedSoCanJumpCancelNow() const { return (*(DWORD*)(ent + 0x240) & 0x100000) != 0; }
	inline bool destroyOnPlayerHitstun() const { return (*(DWORD*)(ent + 0x240) & 0x2000000) != 0; }
	inline bool enableAirtech() const { return (*(DWORD*)(ent + 0x4d40) & 0x4) != 0; }
	inline bool enableWhiffCancels() const { return (*(DWORD*)(ent + 0x4d48) & 0x2) != 0; }
	inline bool enableSpecialCancel() const { return (*(DWORD*)(ent + 0x4d48) & 0x4) != 0; }
	inline bool enableJumpCancel() const { return (*(DWORD*)(ent + 0x4d48) & 0x8) != 0; }
	inline bool enableWalkForward() const { return (*(DWORD*)(ent + 0x4d3c) & 0x4) != 0; }
	inline bool enableJump() const { return (*(DWORD*)(ent + 0x4d3c) & 0x100) != 0; }
	inline bool enableAirOptions() const { return (*(DWORD*)(ent + 0x4d3c) & 0x400) != 0; }
	inline bool enableNormals() const { return (*(DWORD*)(ent + 0x4d3c) & 0x1000) != 0; }
	inline bool enableSpecials() const { return (*(DWORD*)(ent + 0x4d3c) & 0x2000) != 0; }
	inline bool enableGatlings() const { return (*(DWORD*)(ent + 0x4d48) & 0x1) != 0; }
	inline DWORD forceDisableFlags() const { return *(DWORD*)(ent + 0x24e3c); }
	inline bool enableBlock() const { return (*(DWORD*)(ent + 0x4d3c) & 0x10000) != 0; }
	inline int prohibitFDTimer() const { return *(int*)(ent + 0x4d60); }
	inline int dustGatlingTimer() const { return *(int*)(ent + 0x2ce40); }  // for hotizontal dust only
	inline int fdNegativeCheck() const { return *(int*)(ent + 0x2ce60); }
	inline int slowSpeedPercentage() const { return *(int*)(ent + 0x2ce64); }
	inline bool enableCrouch() const { return (*(DWORD*)(ent + 0x4d3c) & 0x2) != 0; }
	inline bool enableWalkBack() const { return (*(DWORD*)(ent + 0x4d3c) & 0x20) != 0; }
	inline int lifeTimeCounter() const { return *(int*)(ent + 0x18); }
	// Jitabata (stagger), Hajikare (rejection), and every regular hitstun animation are treated as hitstun. Bursting, Kizetsu (dizzy), airtech and wakeup are not hitstun.
	// The reason Jitabata has no throw protection is because it is a hardcoded exception to the throw protection rule.
	// And Burst causes RRC because it's a hardcoded exception to RC rules
	inline bool inHitstun() const { return (*(DWORD*)(ent + 0x23c) & 0x6) != 0; }
	inline bool inHitstunThisFrame() const { return (*(DWORD*)(ent + 0x23c) & 0x4) != 0; }
	inline int comboCount() const { return *(int*)(ent + 0x9F28); }  // this is set on the one getting combo'd
	inline bool running() const { return (*(DWORD*)(ent + 0x4d28) & 0x400) != 0; }
	inline bool isCpu() const { return (*(DWORD*)(ent + 0x4d28) & 0x1000) != 0; }
	inline bool gettingUp() const { return (*(DWORD*)(ent + 0x4d28) & 0x4000) != 0; }
	inline bool infiniteKd() const { return (*(DWORD*)(ent + 0x4d28) & 0x80000) != 0; }
	inline bool airthrowDisabled() const { return (*(DWORD*)(ent + 0x4d28) & 0x400000) != 0; }
	inline bool defaultYrcWindowOver() const { return (*(DWORD*)(ent + 0x4d28) & 0x8000000) != 0; }  // this is for moves that don't use yrcWindowLength
	inline bool overridenYrcWindowOver() const { return (*(DWORD*)(ent + 0x4d28) & 0x10000000) != 0; }  // but this is only for moves that use yrcWindowLength
	inline Entity previousEntity() const { return Entity{*(char**)(ent + 0x208)}; }  // the last created entity by this entity
	inline Entity stackEntity(int index) const { return Entity{*(char**)(ent + 0x210 + index * 4)}; }  // STACK_0..7 from bbscript
	inline int& scaleX() const { return *(int*)(ent + 0x264); }
	inline int& scaleY() const { return *(int*)(ent + 0x268); }
	inline int& scaleZ() const { return *(int*)(ent + 0x26c); }
	inline int& scaleDefault() const { return *(int*)(ent + 0x2594); }
	inline int defendersRisc() const { return *(int*)(ent + 0x25b0); }
	inline int forceDisableFlagsIndividual() const { return *(int*)(ent + 0x25c0); }  // these flags combine from all entities belonging to a player's team into the player's main entity's forceDisableFlags()
	inline int& scaleDefault2() const { return *(int*)(ent + 0x2664); }
	inline int& scaleForParticles() const { return *(int*)(ent + 0x2614 + 0x4); }
	inline int speedX() const { return *(int*)(ent + 0x2fc); }
	inline int& speedY() const { return *(int*)(ent + 0x300); }
	inline int dashSpeed() const { return *(int*)(ent + 0x2f4); }
	inline int gravity() const { return *(int*)(ent + 0x304); }
	inline int comboTimer() const { return *(int*)(ent + 0x9f50); }
	inline BOOL riscMinusOnceUsed() const { return *(BOOL*)(ent + 0x9f54); }
	inline BOOL rcDmgProration() const { return *(BOOL*)(ent + 0x9f5c); }
	inline int fdPushback() const { return *(int*)(ent + 0x31c); }  // Ky 5H on May FD ground block, Ky gets -900 on first frame of hitstop. facing right
	inline int pushback() const { return *(int*)(ent + 0x2cc); }  // Ky punch May with 5P, May gets pushback 20020 on the frame after hitstop ends. = pendingPushback * 175 / 10
	inline int pendingPushback() const { return *(int*)(ent + 0x318); }  // Ky punch May with 5P, May gets pushback 1144 on the frame after hitstop ends.
	inline bool ascending() const { return (*(DWORD*)(ent + 0x234) & 0x1) != 0; }  // this does not mean prejump. It is set on the initial 7 frames of May jump, 10 Ky jump.
	                                                                         // Those are the frames when your sprite isn't changing, it changes as soon as flag gets unset.
	inline bool displayModel() const { return *(bool*)(ent + 0x2814); }
	inline BBScrEvent signalToSendToYourOwnEffectsWhenHittingThem() const { return *(BBScrEvent*)(ent + 0x283c); }  // bbscript: naguriNaguru
	inline bool hideUI() const { return (*(DWORD*)(ent + 0x11c) & 0x4000) != 0; }
	inline bool isHidden() const { return (*(DWORD*)(ent + 0x11c) & 0x40000000) != 0; }
	inline bool isRecoveryState() const { return (*(DWORD*)(ent + 0x234) & 0x40000000) != 0; }
	inline int playerVal(int n) const { return *(int*)(ent + 0x24c50 + 4 * n); }  // all get reset to 0 on stage reset
	inline HitEffect currentHitEffect() const { return *(HitEffect*)(ent + 0x24db0); }  // this is an enum, all values of which are not fully understood, so we're not writing it
	inline int airdashHorizontallingTimer() const { return *(int*)(ent + 0x24db8); }
	inline int cantBackdashTimer() const { return *(int*)(ent + 0x24dbc); }
	inline TeamSwap teamSwap() const { return *(TeamSwap*)(ent + 0x26ac); }
	inline int currentHitNum() const { return *(int*)(ent + 0x26d8); }  // increments even when not landing any hits. Restarts from 0 with each move
	inline const AttackData* dealtAttack() const { return (const AttackData*)(ent + 0x44c); }
	inline const AttackData* receivedAttack() const { return (const AttackData*)(ent + 0x710); }
	// Starts at 2560 on Sol getting dizzied.
	// On next frame decreases by 10 if you didn't press a button.
	// On next frame doesn't decrease because you're in 27f hitstop.
	// Starting on the frame hitstop reaches 0, including that frame, decreases by 10 every frame.
	// When it reaches 0, you're free from dizziness.
	// Is 0 on stagger hits (Ky CH 5H for ex.) and is not used for stagger mashing
	inline int dizzyMashAmountLeft() const { return *(int*)(ent + 0x9fcc); }
	inline int dizzyMashAmountMax() const { return *(int*)(ent + 0x9fd0); }
	inline int exKizetsu() const { return *(int*)(ent + 0x24dc4); }  // special-faint (DI recovery)
	// these variables mean different things in different CmnAct animations
	// CmnActJitabataLoop: 0 or 1 - has started the 4f recovery animation
	// CmnActKorogari: 0 or 2. On 1 waits for currentAnimDuration() to reach tumble, then proceeds to 2.
	//                 On 2 waits 30 frames, then proceeds to CmnActBDownLoop
	inline int bbscrvar() const { return *(int*)(ent + 0x24df4); }
	// CmnActJitabataLoop: ongoing recovery animation duration so far. Maximum 5. On 6 you should be in neutral already
	// CmnActRomanCancel: RC startup. Gets decremented when bbscrvar is 0 until it reaches 0, then bbscrvar becomes 1
	// CmnActKorogari: this counts time to 30 when bbscrvar() is 2
	inline int bbscrvar2() const { return *(int*)(ent + 0x24df8); }
	// CmnActJitabataLoop: initially set to half the staggerDuration. If current animation duration reaches this, the combo is graybeat,
	// and you can't mash bbscrvar5 below this. bbscrvar3 does not change
	inline int bbscrvar3() const { return *(int*)(ent + 0x24dfc); }
	// CmnActRomanCancel: pending slowdown (YRC - 19, PRC - 40, RRC - 60)
	inline int bbscrvar4() const { return *(int*)(ent + 0x24e00); }
	// CmnActJitabataLoop: initially set to staggerDuration * 10. This / 10 is compared to current animation duration to determine if recovery animation should play.
	// it is reduced every frame there's a PKSHD button press by 30
	// CmnActRomanCancel: custom frame counter that counts the number of idling signals. Is incremented at the end of idling handler
	inline int bbscrvar5() const { return *(int*)(ent + 0x24e04); }
	inline int bbscrvar6() const { return *(int*)(ent + 0x24e08); }
	inline int damageScale() const { return *(int*)(ent + 0x24d74); }  // Raven uses it on non-0 excitement. This value on the attacker player
	inline int projectileDamageScale() const { return *(int*)(ent + 0x2530); }  // this value on the defender
	inline int superArmorDamagePercent() const { return *(int*)(ent + 0x9b8); }  // this value on the defender
	inline BOOL increaseDmgBy50Percent() const { return *(BOOL*)(ent + 0x24cac); }  // no idea. This value on the defender
	inline int extraInverseProration() const { return *(int*)(ent + 0x24d78); } // this value on the defender
	inline int dustProration1() const { return *(int*)(ent + 0x2ce50); }  // this value on the defender
	inline int dustProration2() const { return *(int*)(ent + 0x2ce54); }  // this value on the defender
	inline int hitstunOrBlockstunTypeKindOfState() const { return *(int*)(ent + 0x2ce5c); }
	inline bool hellfireState() const { return (*(DWORD*)(ent + 0x4d2c) & 0x4) != 0; }  // this value on the attacker player, but must also check health <= 10%
	inline bool performingSuperOrIK() const { return (*(DWORD*)(ent + 0x4d2c) & 0x4000) != 0; }
	inline bool landed() const { return (*(int*)(ent + 0x234) & 0x4) != 0; }  // is true for only one frame - the frame on which you touched the ground
	inline bool hitSomethingOnThisFrame() const { return (*(int*)(ent + 0x12c) & 0x20000) != 0; }  // is true for only one frame - the frame on which you hit something
	inline bool receivedProjectileClashSignal() const { return (*(int*)(ent + 0x12c) & 0x8) != 0; }
	inline bool inHitstunNextFrame() const { return (*(int*)(ent + 0x23c) & 0x2) != 0; }  // is true for only one frame - the frame on which you get hit
	inline bool inHitstunThisOrNextFrame() const { return inHitstun(); }
	inline bool inBlockstunNextFrame() const { return (*(int*)(ent + 0x23c) & 0x1000000) != 0; }  // is true for only one frame - the frame on which you block a hit
	inline Entity currentRunOnObject() const { return *(Entity*)(ent + 0x2464); }
	inline bool successfulIB() const { return (*(DWORD*)(ent + 0x23c) & 0x800000) != 0; }  // can be set even on FD IB. Remains set even after blockstun is over.
	inline HitResult lastHitResult() const { return *(HitResult*)(ent + 0x984); }
	inline DWORD lastHitResultFlags() const { return *(DWORD*)(ent + 0x988); }
	inline bool immuneToSuperfreeze() const { return (*(DWORD*)(ent + 0x118) & 0x4000) != 0; }
	inline bool immuneToAlliedSuperfreeze() const { return (*(DWORD*)(ent + 0x118) & 0x2000000) != 0; }
	inline bool isTouchingLeftWall() const { return (*(DWORD*)(ent + 0x118) & 0x400000) != 0; }
	inline bool isTouchingRightWall() const { return (*(DWORD*)(ent + 0x118) & 0x800000) != 0; }
	inline bool isTouchingLeftScreenEdge() const { return (*(DWORD*)(ent + 0x118) & 0x100000) != 0; }
	inline bool isTouchingRightScreenEdge() const { return (*(DWORD*)(ent + 0x118) & 0x200000) != 0; }
	inline bool isSuperFrozen() const { return (*(DWORD*)(ent + 0x118) & 0x4000000) != 0; }
	inline bool isRCFrozen() const { return (*(DWORD*)(ent + 0x11c) & 0x200000) != 0; }  // true on every second frame of RC slowdown, when that frame got skippped
	inline Entity attacker() const { return *(Entity*)(ent + 0x708); }
	inline bool holdingFD() const { return (*(DWORD*)(ent + 0x23c) & 0x20000000) != 0; }
	inline bool superArmorEnabled() const { return (*(DWORD*)(ent + 0x9a4) & 0x2) != 0; }  // by default super armor tanks everything
	inline SuperArmorType superArmorType() const { return *(SuperArmorType*)(ent + 0x9a8); }
	inline bool superArmorThrow() const { return (*(DWORD*)(ent + 0x9a4) & 0x40) != 0; }
	inline bool superArmorBurst() const { return (*(DWORD*)(ent + 0x9a4) & 0x200) != 0; }
	inline bool superArmorMid() const { return (*(DWORD*)(ent + 0x9a4) & 0x400) != 0; }
	inline bool superArmorOverhead() const { return (*(DWORD*)(ent + 0x9a4) & 0x800) != 0; }
	inline bool superArmorLow() const { return (*(DWORD*)(ent + 0x9a4) & 0x1000) != 0; }
	inline bool superArmorGuardImpossible() const { return (*(DWORD*)(ent + 0x9a4) & 0x2000) != 0; }
	inline bool superArmorObjectAttacck() const { return (*(DWORD*)(ent + 0x9a4) & 0x20000) != 0; }  // this is not a typo, it's from bbscript
	inline bool superArmorHontaiAttacck() const { return (*(DWORD*)(ent + 0x9a4) & 0x40000) != 0; }  // this is not a typo, it's from bbscript
	inline bool superArmorProjectileLevel0() const { return (*(DWORD*)(ent + 0x9a4) & 0x200000) != 0; }
	inline bool superArmorForReflect() const { return (*(DWORD*)(ent + 0x9a4) & 0x100000) != 0; }
	inline bool superArmorOverdrive() const { return (*(DWORD*)(ent + 0x9a4) & 0x400000) != 0; }
	inline bool superArmorBlitzBreak() const { return (*(DWORD*)(ent + 0x9a4) & 0x1000000) != 0; }
	// having this flag drastically reduces your super armor to only hits that can be reflected.
	// Having this flag without superArmorEnabled is useless because you just get hit by the projectile
	inline bool invulnForAegisField() const { return (*(DWORD*)(ent + 0x238) & 0x400) != 0; }
	inline bool hasUpon(BBScrEvent index) const { return ((BitArray<3>*)(ent + 0xa0c))->getBit(index); }  // means that an event handler statement block must be executed upon event, address of the block specified in UponInfo::uponInstrPtr
	inline bool needGoToStateUpon(BBScrEvent index) const { return ((BitArray<3>*)(ent + 0xa18))->getBit(index); }  // name of the state specified in UponInfo::stateToGoTo
	inline bool needGoToMarkerUpon(BBScrEvent index) const { return ((BitArray<3>*)(ent + 0xa24))->getBit(index); }  // name of the marker specified in UponInfo::markerToGoTo
	const UponInfo* uponStruct(BBScrEvent index) const { return (const UponInfo*)(ent + 0xb70) + index; }
	inline int mem45() const { return *(int*)(ent + 0x14c); }  // Reset on state change
	inline int mem46() const { return *(int*)(ent + 0x150); }  // Reset on state change
	inline int mem47() const { return *(int*)(ent + 0x154); }  // Reset on state change
	inline int mem48() const { return *(int*)(ent + 0x158); }  // Reset on state change
	inline int mem49() const { return *(int*)(ent + 0x15c); }  // Reset on state change
	inline int mem50() const { return *(int*)(ent + 0x160); }  // Reset on state change
	inline int mem51() const { return *(int*)(ent + 0x164); }  // Reset on state change
	inline int mem52() const { return *(int*)(ent + 0x168); }  // Reset on state change
	inline int mem53() const { return *(int*)(ent + 0x16c); }  // Does not reset on stage change
	inline int mem54() const { return *(int*)(ent + 0x170); }  // Does not reset on stage change
	inline int mem55() const { return *(int*)(ent + 0x174); }  // Does not reset on stage change
	inline int mem56() const { return *(int*)(ent + 0x178); }  // Does not reset on stage change
	inline int mem57() const { return *(int*)(ent + 0x17c); }  // Does not reset on stage change
	inline int mem58() const { return *(int*)(ent + 0x180); }  // Does not reset on stage change
	inline int mem59() const { return *(int*)(ent + 0x184); }  // Does not reset on stage change
	inline int mem60() const { return *(int*)(ent + 0x188); }  // Does not reset on stage change
	inline int mem201() const { return *(int*)(ent + 0x24c60); }  // idk
	inline int mem202() const { return *(int*)(ent + 0x24c64); }
	inline int mem203() const { return *(int*)(ent + 0x24c68); }
	
	// bbscript numbers them from 1, I number them from 1 too, so type in the number you see in bbscript
	// indices 1-4 are reset on state change, 5-8 are not
	inline int storage(int n) const { return *(int*)(ent + 0x18c + 4 * (n - 1)); }  
	inline int exGaugeValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0x10); }  // reset to 0 on stage reset
	inline int exGaugeMaxValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0xc); }  // reset to 0 on stage reset
	inline char* gotoLabelRequests() const { return (char*)(ent + 0x2474 + 0x24); }  // on the next frame, go to marker named this, within the same state. Note: may go on the same frame, if the event you're calling this from happens before animations advance
	inline char* spriteName() const { return (char*)(ent + 0xa58); }
	inline int spriteFrameCounter() const { return *(int*)(ent + 0xa78); }
	inline int spriteFrameCounterMax() const { return *(int*)(ent + 0xa80); }
	inline bool justReachedSprite() const { return !isRCFrozen() && spriteFrameCounter() == 0; }
	// If playing a sprite, points to the next sprite command that would go after it.
	// When on the last sprite, points to the endState instruction.
	// When on a sprite, but a spriteEnd command is after it, points to the instruction after spriteEnd.
	inline BYTE* bbscrCurrentInstr() const { return *(BYTE**)(ent + 0xa50); }
	inline BYTE* bbscrCurrentFunc() const { return *(BYTE**)(ent + 0xa54); }  // points to a beginState instruction. Can be null inside of pawnInitializeHook
	inline int remainingDoubleJumps() const { return *(int*)(ent + 0x4d58); }
	inline int remainingAirDashes() const { return *(int*)(ent + 0x4d5c); }
	inline int maxAirdashes() const { return *(int*)(ent + 0x9884); }
	inline int maxDoubleJumps() const { return *(int*)(ent + 0x9888); }
	inline Entity playerEntity() const { return *(Entity*)(ent + 0x1d0); }
	inline Entity parentEntity() const { return *(Entity*)(ent + 0x1d4); }
	inline Entity enemyEntity() const { return *(Entity*)(ent + 0x1d8); }
	inline Entity effectLinkedCollision() const { return *(Entity*)(ent + 0x204); }  // bbscript: linkObjectCollision
	inline int pitch() const { return *(int*)(ent + 0x258); }  // 1000 means one degree counter-clockwise, if facing right. Gets mirrored with facing, so when facing left, it's clockwise instead
	inline int transformCenterX() const { return *(int*)(ent + 0x27c); }  // does not depend on sprite facing, does not get mirrored with sprite facing
	inline int transformCenterY() const { return *(int*)(ent + 0x280); }  // does not depend on sprite facing, does not get mirrored with sprite facing
	HitboxHolder* hitboxes() const { return (HitboxHolder*)(ent + 0x48); }
	inline const InflictedParameters* inflicted() const { return (const InflictedParameters*)(ent + 0x67c); }
	inline const InflictedParameters* inflictedCH() const { return (const InflictedParameters*)(ent + 0x6c0); }
	inline const InflictedParameters* received() const { return (const InflictedParameters*)(ent + 0x940); }
	inline bool isOtg() const { return (*(DWORD*)(ent + 0x4d24) & 0x800000) != 0; }
	inline bool damageToAir() const { return (*(DWORD*)(ent + 0x4d24) & 0x8000) != 0; }  // this is present on Answer Backdash, Faust Pogo, May Horizontal Dolphin first few frames, etc
	inline bool crouching() const { return (*(DWORD*)(ent + 0x4d24) & 0x1) != 0; }
	inline bool setOnCmnActDownBoundEntry() const { return (*(DWORD*)(ent + 0x4d24) & 2) != 0; }  // this is set when entering CmnActDownBound animation. Also present in all of hitstun animations
	inline bool lying() const { return (*(DWORD*)(ent + 0x4d24) & 0x4) != 0; }  // this could mean two things
	inline int strikeInvulnFrames() const { return *(int*)(ent + 0x9a0); }
	inline int throwInvulnFrames() const { return *(int*)(ent + 0x99c); }
	inline bool strikeInvul() const { return (*(DWORD*)(ent + 0x238) & 0x10) != 0; }
	inline bool throwInvul() const { return (*(DWORD*)(ent + 0x238) & 0x20) != 0; }
	inline bool fullInvul() const { return (*(DWORD*)(ent + 0x238) & 0x40) != 0; }  // all projectiles by default will have this flag
	inline int thisIsMinusOneIfEnteredHitstunWithoutHitstop() const { return *(int*)(ent + 0x262dc); }  // 0 otherwise
	inline int createArgHikitsukiVal1() const { return *(int*)(ent + 0x2660 + 0x34); }
	inline int createArgHikitsukiVal2() const { return *(int*)(ent + 0x2660 + 0x38); }
	// this is > 0 in hitstun, blockstun,
	// including 6f after hitstun, 5f after blockstun and 9f after wakeup
	inline int throwProtection() const { return *(unsigned int*)(ent + 0x9fE4) > 0; }
	inline bool yellowRomanCancel() const { return (*(DWORD*)(ent + 0x24c0 + 0x20) & 0x800000) != 0; }
	inline bool purpleRomanCancel() const { return (*(DWORD*)(ent + 0x24c0 + 0x20) & 0x1000000) != 0; }
	inline int currentMoveIndex() const { return *(int*)(ent + 0x24c0 + 0x44); }  // currentMoveIndex() MAY BE -1!!!
	inline int* moveIndices() const { return (int*)(ent + 0xa020 + 0x16530); }  // used to iterate moves
	inline int moveIndicesCount() const { return *(int*)(ent + 0xa020 + 0x16800); }  // iterate from the end (count - 1) to 0
	inline const AddedMoveData* movesBase() const { return (const AddedMoveData*)(ent + 0xa020); }
	inline int movesCount() const { return *(int*)(ent + 0xa020 + 0x18748); }
	inline const AddedMoveData* currentMove() const { return movesBase() + currentMoveIndex(); }  // currentMoveIndex() MAY BE -1!!!
	inline int hitAlreadyHappened() const { return *(int*)(ent + 0x444); }  // equal to 10 when occured, 0 when not
	inline int theValueHitAlreadyHappenedIsComparedAgainst() const { return *(int*)(ent + 0x448); }  // always 10
	inline int crossupProtection() const { return *(int*)(ent + 0xa010); }
	inline bool immuneToRCSlowdown() const { return (*(DWORD*)(ent + 0x11c) & 0x10000000) != 0; }
	inline int rcSlowdownCounter() const { return *(int*)(ent + 0x261fc); }
	inline int poisonDuration() const { return *(int*)(ent + 0x2510); }
	// some moves have this at first and then decide to get rid of it: Mad Struggle, Eddie K at a wall (sets to an impossibly high value).
	// Other moves don't have it and then obtain it: Jam j.2K
	inline int maxHit() const { return *(int*)(ent + 0x25a4); }
	inline int exPointX() const { return *(int*)(ent + 0x25a8); }
	inline int exPointY() const { return *(int*)(ent + 0x25ac); }
	inline int numberOfHitsTaken() const { return *(int*)(ent + 0x253c); }
	inline int numberOfHits() const { return *(int*)(ent + 0x2540); }
	inline bool notDestroyOnMaxNumOfHits() const { return (*(DWORD*)(ent + 0x240) & 0x20000000) != 0; }
	inline bool destroyOnPlayerStateChange() const { return (*(DWORD*)(ent + 0x240) & 0x1000000) != 0; }
	inline int destroyOnBlockOrArmor() const { return *(int*)(ent + 0x2548); }
	inline int destroyOnHitProjectile() const { return *(int*)(ent + 0x254c); }
	inline int destroyOnHitPlayer() const { return *(int*)(ent + 0x2544); }
	inline int guardBalanceDefence() const { return *(int*)(ent + 0x98a4); }
	inline int blockCount() const { return *(int*)(ent + 0x9fe0); }
	inline CounterHitEntityValue receivedCounterHit() const { return *(CounterHitEntityValue*)(ent + 0x990); }
	inline bool counterHitState() const { return (*(DWORD*)(ent + 0x234) & 0x100) != 0; }  // Thanks to WorseThanYou for finding this
	inline bool jitabataLoop() const { return (*(DWORD*)(ent + 0x234) & 0x20000000) != 0; }
	inline bool ignoreRcProration() const { return *(BOOL*)(ent + 0x26200) != 0; }
	inline TensionMode tensionMode() const { return *(TensionMode*)(ent + 0x2621c); }
	inline int dustPropulsion() const { return *(int*)(ent + 0x24de8); }  // a timer that decrements. For vertical dust only
	inline int unknownField1() const { return *(int*)(ent + 0x24df0); }
	inline int burstGainOnly20Percent() const { return *(int*)(ent + 0x2d130); }
	inline RomanCancelAvailability romanCancelAvailability() const { return *(RomanCancelAvailability*)(ent + 0x26214); }
	inline int gtmpX() const { return *(int*)(ent + 0x262bc); }
	inline int gtmpY() const { return *(int*)(ent + 0x262c0); }
	inline int mem64() const { return *(int*)(ent + 0x262c8); }
	inline BBScrInfo* bbscrInfo() const { return *(BBScrInfo**)(ent + 0xa48); }
	BYTE* findStateStart(const char* name) const;
	BYTE* findSubroutineStart(const char* name) const;
	inline BOOL destructionRequested() const { return *(BOOL*)(ent + 0x2538); }
	inline int landingHeight() const { return *(int*)(ent + 0x1cc); }
	int getCenterOffsetY() const;
	inline Entity linkObjectDestroyOnStateChange() const { return *(Entity*)(ent + 0x1f0); }
	inline Entity linkObjectDestroyOnDamage() const { return *(Entity*)(ent + 0x1ec); }
	inline Entity stopLinkObject() const { return *(Entity*)(ent + 0x1f4); }
	inline int venomBallArg3() const { return *(int*)(ent + 0x25bc); }
	inline int createArgHikitsukiVal1_outgoing() const { return *(int*)(ent + 0x2614 + 0x34); }  // gets reset to 0 after creating an object
	inline int createArgHikitsukiVal2_outgoing() const { return *(int*)(ent + 0x2614 + 0x38); }  // gets reset to 0 after creating an object
	inline const char* previousAnimName() const { return (const char*)(ent + 0x2424); }
	inline int airDashMinimumHeight() const { return *(int*)(ent + 0x9864); }
	inline int framesSinceRegisteringForTheIdlingSignal() const { return *(int*)(ent + 0x140); }  // unfortunately, this value is +1 at the end of the tick more than what it was during an idling event. Luckily, you can just -1 from it before using
	inline int clashOrRCBufferTimer() const { return *(int*)(ent + 0x24dcc); }  // decrements by 1 in the big function that uses the "cmn_MOM_Jibaku" string. When not 0, adds bufferTimeFromRC+13 to buffer window of moves
	inline int dustHomingJump1BufferTimer() const { return *(int*)(ent + 0x24dd0); }  // set to 100 by the C++ side of the code of the callPrivateFunction: s32'DustHomingJump1' in bbscript in cmnHomingJumpInitialize subroutine, which only happens on 5D8, not 5D6
	inline int blitzShieldHitstopBuffer() const { return *(int*)(ent + 0x24dd4); }
	inline int bufferTimeFromRC() const { return *(int*)(ent + 0x24dd8); }
	inline int ensureAtLeast3fBufferForNormalsWhenJumping() const { return *(int*)(ent + 0x262f4); }
	inline const ScheduledAnim* currentAnimData() const { return (ScheduledAnim*)(ent + 0x24c0); }
	inline const ScheduledAnim* nextAnim() const { return (ScheduledAnim*)(ent + 0x2474); }
	inline int TrainingEtc_ComboDamage() const { return *(int*)(ent + 0x9f44); }
	
	void getState(EntityState* state, bool* wasSuperArmorEnabled = nullptr, bool* wasFullInvul = nullptr) const;
	
	static void getWakeupTimings(CharacterType charType, WakeupTimings* output);
	void getWakeupTimings(WakeupTimings* output) const;
	int calculateGuts(int* gutsLevel = nullptr) const;
	
	inline CmnActHashtable* addedMovesHashtable() const { return (CmnActHashtable*)(ent + 0xa020 + 0x18750); }
	const AddedMoveData* findAddedMove(const char* name) const;
	static DWORD hashStringLowercase(const char* str);
	static DWORD hashString(const char* str);
	
	inline DWORD burstGainCounter() const { return *(DWORD*)(ent + 0x2ce4c); }
	inline DWORD venomBallFlags() const { return *(DWORD*)(ent + 0x25bc); }
	inline bool canTriggerBalls() const { return (venomBallFlags() & 0x4) != 0; }  // can trigger Venom Balls
	inline bool venomBallSpin() const { return (venomBallFlags() & 0x30) != 0; }
	inline int venomBallArg2() const { return *(int*)(ent + 0x25b8); }
	
	inline float attackY() const { return *(float*)(ent + 0x9c8); }
	
	inline int purpleHealthTimer() const { return *(int*)(ent + 0x2ce6c); }
	inline int purpleHealth() const { return *(int*)(ent + 0x2ce68); }
	
	inline REDPawn* pawnWorld() const { return *(REDPawn**)(ent + 0x27a8); }
	inline FPAC*& fpac() const { return *(FPAC**)(ent + 0x114); }
	inline int bbscrIndexInAswEng() const { return *(int*)(ent + 0x3c); }
	REDAssetCollision* getCollision() const;
	int getEffectIndex() const;
	inline bool showPushbox() const { return isPawn() || servant() || ghost(); }
	inline char(&charCodename())[16] { return *(char(*)[16])(ent + 0x4d10); }
	
	inline char* operator+(int offset) const { return (char*)(ent + offset); }
	inline char* operator+(DWORD offset) const { return (char*)(ent + offset); }

	inline bool operator==(const Entity& other) const { return ent == other.ent; }
	inline bool operator!=(const Entity& other) const { return ent != other.ent; }
	inline bool operator==(void* other) const { return (void*)ent == (void*)other; }
	inline bool operator!=(void* other) const { return (void*)ent != (void*)other; }
	
	inline operator bool() const { return ent != nullptr; }
	inline operator void*() const { return (void*)ent; }
	inline operator char*() const { return ent; }

};

class EntityManager {
public:
	bool onDllMain();
	int calculateExtraTensionGainModifier(void* pawn);
	char calculateTensionPulsePenaltySeverity(int tensionPulsePenalty);
	char calculateCornerPenaltySeverity(int cornerPenalty);
	void calculateTensionGainModifier(
		int distance,
		int negativePenaltyTimer,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPenaltyModifier,
		int* tensionPulseModifier);
	void calculateTensionPulsePenaltyGainModifier(
		int distance,
		int tensionPulse,
		int* distanceModifier,
		int* tensionPulseModifier);
	int calculateReceivedComboCountTensionGainModifier(bool inHitstun, int comboCount);
	int calculateDealtComboCountTensionGainModifier(bool inHitstun, int comboCount);
	void calculatePushback(
		int attackLevel,
		int comboTimer,
		bool dontUseComboTimer,
		bool ascending,
		int y,
		int pushbackModifier,
		int airPushbackModifier,
		bool inHitstunOrInHitstunNextFrame,
		int pushbackModifierOnHitstun,
		int* basePushback,
		int* attackPushbackModifier,
		int* hitstunPushbackModifier,
		int* comboTimerPushbackModifier);
	void calculateSpeedYProration(
		int comboCount,
		int weight,
		bool ignoreWeight,
		bool disableComboProtation,
		int* weightModifier,
		int* comboCountModifier);
	void calculateHitstunProration(
		bool noHitstunScaling,
		bool isInAir,
		int comboTimer,
		int* hitstunProration);
	int calculateComboProration(int risc, AttackType attackType);  // from 0 to 256
private:
	friend class Entity;
	getPos_t getPosX;
	getPos_t getPosY;
	getPushboxCoords_t getPushboxCoords;
	getExtraTensionModifier_t getExtraTensionModifier = nullptr;
};

extern EntityManager entityManager;
