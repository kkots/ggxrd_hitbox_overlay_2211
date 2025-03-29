#pragma once
#include "characterTypes.h"
#include "Hitbox.h"

using getPos_t = int(__thiscall*)(const void*);
using getPushbox_t = int(__thiscall*)(const void*);
using getExtraTensionModifier_t = int(__thiscall*)(const void*, int param1);

enum HitboxType : char {
	HITBOXTYPE_HURTBOX,
	HITBOXTYPE_HITBOX,
	HITBOXTYPE_EX_POINT,  // used in some animations like Millia Bad Moon
	HITBOXTYPE_EX_POINT_EXTENDED,
	HITBOXTYPE_TYPE4,
	HITBOXTYPE_TYPE5,
	HITBOXTYPE_TYPE6,
	HITBOXTYPE_NECK,
	HITBOXTYPE_ABDOMEN,
	HITBOXTYPE_R_LEG,
	HITBOXTYPE_L_LEG,
	HITBOXTYPE_PRIVATE0,  // Bedman: boomerang head calculates its distance to this point of Bedman
	HITBOXTYPE_PRIVATE1,
	HITBOXTYPE_PRIVATE2,
	HITBOXTYPE_PRIVATE3,
	HITBOXTYPE_TYPE17
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
	CmnActExDamageLand = 0x5d,  // don't know
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
	DWORD conditions[5];
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
	bool hasCondition(MoveCondition condition) const;
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
	int receivedAtkLvlBlockstun;
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
	int attackKezuri;
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
	char stateToGoTo[32];  // used if bbscrNeedGoToStateUpon is set. Name of anim
	int fireOnThisValue;  // for upon 0xd it's frame number, for upon 0x4 it's speedY
	int fireOnThisValue2;
	char markerToGoTo[32];  // used if bbscrNeedGoToMarkerUpon is set. Marker name
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

class Entity
{
public:
	inline Entity() {}
	inline Entity(void* ent) { this->ent = (char*)ent; }
	char* ent = nullptr;
	
	inline bool isActive() const { return *(DWORD*)(ent + 0xc) == 1; }
	// Active means attack frames are coming
	inline bool isActiveFrames() const {
		return (*(DWORD*)(ent + 0x23C) & 0x100) != 0  // This signals that attack's hitboxes should not be ignored. Can happen before hitboxes come out
			&& (*(DWORD*)(ent + 0x234) & 0x40000000) == 0;  // This signals that attack's hitboxes should be ignored.
		                                                           // Can be simultaneous with 0x100 flag in 0x23C - recovery takes priority
		                                                           // Some moves don't have this flag during their recovery
	}
	inline bool hasActiveFlag() const { return (*(DWORD*)(ent + 0x23C) & 0x100) != 0; }
	
	// 0 for P1, 1 for P2
	inline char team() const { return *(char*)(ent + 0x40); }
	
	inline bool isPawn() const { return *(bool*)(ent + 0x10); }
	inline CharacterType characterType() const { return (CharacterType)*(char*)(ent + 0x44); }
	
	inline int x() const { return *(int*)(ent + 0x24c); }
	inline int y() const { return *(int*)(ent + 0x250); }
	inline int& x() { return *(int*)(ent + 0x24c); }
	inline int& y() { return *(int*)(ent + 0x250); }
	// This function is for hitbox calculation only. To obtain raw x position, use x()
	int posX() const;
	// This function is for hitbox calculation only. To obtain raw y position, use y()
	int posY() const;
	
	inline bool isFacingLeft() const { return *(int*)(ent + 0x248) == 1; }  // the graphical facing
	inline BOOL& isFacingLeft() { return *(BOOL*)(ent + 0x248); }  // the graphical facing
	inline BOOL& inputsFacingLeft() { return *(BOOL*)(ent + 0x4d38); }  // the facing for input motions interpreting

	bool isGettingThrown() const;

	int pushboxWidth() const;
	int pushboxTop() const;
	// Everyone always seems to have this value set to 0 no matter what
	inline int pushboxFrontWidthOffset() const { return *(int*)(ent + 0x32C); }
	int pushboxBottom() const;

	void pushboxLeftRight(int* left, int* right) const;
	
	inline bool performingThrow() const { return (*(DWORD*)(ent + 0x23c) & 0x1000) != 0; }  // this flag appears when starting a throw or throw-like move and stays until first hit connects
	inline unsigned int currentAnimDuration() const { return *(const unsigned int*)(ent + 0x130); }
	inline const char* animationName() const { return (const char*)(ent + 0x2444); }
	inline CmnActIndex cmnActIndex() const { return *(CmnActIndex*)(ent + 0xa01c); }
	inline bool naguriNagurareru() const { return (*(DWORD*)(ent + 0x120) & 0x100) != 0; }
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
	inline int hitstun() const { return *(int*)(ent + 0x9808); }
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
	inline bool enableAirtech() const { return (*(DWORD*)(ent + 0x4d40) & 0x4) != 0; }
	inline bool enableWhiffCancels() const { return (*(DWORD*)(ent + 0x4d48) & 0x2) != 0; }
	inline bool enableSpecialCancel() const { return (*(DWORD*)(ent + 0x4d48) & 0x4) != 0; }
	inline bool enableJumpCancel() const { return (*(DWORD*)(ent + 0x4d48) & 0x8) != 0; }
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
	inline int& scaleX() { return *(int*)(ent + 0x264); }
	inline int scaleX() const { return *(int*)(ent + 0x264); }
	inline int& scaleY() { return *(int*)(ent + 0x268); }
	inline int scaleY() const { return *(int*)(ent + 0x268); }
	inline int& scaleZ() { return *(int*)(ent + 0x26c); }
	inline int scaleZ() const { return *(int*)(ent + 0x26c); }
	inline int& scaleDefault() { return *(int*)(ent + 0x2594); }
	inline int scaleDefault() const { return *(int*)(ent + 0x2594); }
	inline int defendersRisc() const { return *(int*)(ent + 0x25b0); }
	inline int forceDisableFlagsIndividual() const { return *(int*)(ent + 0x25c0); }  // these flags combine from all entities belonging to a player's team into the player's main entity's forceDisableFlags()
	inline int& scaleDefault2() { return *(int*)(ent + 0x2664); }
	inline int scaleDefault2() const { return *(int*)(ent + 0x2664); }
	inline int& scaleForParticles() { return *(int*)(ent + 0x2614 + 0x4); }
	inline int scaleForParticles() const { return *(int*)(ent + 0x2614 + 0x4); }
	inline int speedX() const { return *(int*)(ent + 0x2fc); }
	inline int speedY() const { return *(int*)(ent + 0x300); }
	inline int& speedY() { return *(int*)(ent + 0x300); }
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
	inline int signalToSendToYourOwnEffectsWhenHittingThem() const { return *(bool*)(ent + 0x283c); }
	inline bool hideUI() const { return (*(DWORD*)(ent + 0x11c) & 0x4000) != 0; }
	inline bool isHidden() const { return (*(DWORD*)(ent + 0x11c) & 0x40000000) != 0; }
	inline bool isRecoveryState() const { return (*(DWORD*)(ent + 0x234) & 0x40000000) != 0; }
	inline int playerVal(int n) const { return *(int*)(ent + 0x24c50 + 4 * n); }
	inline int airdashHorizontallingTimer() const { return *(int*)(ent + 0x24db8); }
	inline int cantBackdashTimer() const { return *(int*)(ent + 0x24dbc); }
	inline TeamSwap teamSwap() const { return *(TeamSwap*)(ent + 0x26ac); }
	inline int currentHitNum() const { return *(int*)(ent + 0x26d8); }
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
	inline int receivedSpeedY() const { return *(int*)(ent + 0x944); }
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
	bool hasUpon(int index) const;
	const UponInfo* uponStruct(int index) const { return (const UponInfo*)(ent + 0xb70) + index; }
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
	
	// bbscript numbers them from 1, I number them from 0, so subtract 1 from the bbscript number
	// indices 0-3 are reset on state change, 4-7 are not
	inline int storage(int n) const { return *(int*)(ent + 0x18c + 4 * n); }  
	inline int exGaugeValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0x10); }
	inline int exGaugeMaxValue(int n) const { return *(int*)(ent + 0x24cbc + 36 * n + 0xc); }
	inline const char* gotoLabelRequest() const { return (const char*)(ent + 0x2474 + 0x24); }  // on the next frame, go to marker named this, within the same state
	inline const char* spriteName() const { return (const char*)(ent + 0xa58); }
	inline int spriteFrameCounter() const { return *(int*)(ent + 0xa78); }
	inline int spriteFrameCounterMax() const { return *(int*)(ent + 0xa80); }
	// If playing a sprite, points to the next sprite command that would go after it.
	// When on the last sprite, points to the endState instruction.
	// When on a sprite, but a spriteEnd command is after it, points to the instruction after spriteEnd.
	inline BYTE* bbscrCurrentInstr() const { return *(BYTE**)(ent + 0xa50); }
	inline BYTE* bbscrCurrentFunc() const { return *(BYTE**)(ent + 0xa54); }  // points to a beginState instruction
	inline int remainingDoubleJumps() const { return *(int*)(ent + 0x4d58); }
	inline int remainingAirDashes() const { return *(int*)(ent + 0x4d5c); }
	inline int maxAirdashes() const { return *(int*)(ent + 0x9884); }
	inline int maxDoubleJumps() const { return *(int*)(ent + 0x9888); }
	inline Entity playerEntity() const { return *(Entity*)(ent + 0x1d0); }
	inline Entity enemyEntity() const { return *(Entity*)(ent + 0x1d8); }
	inline Entity effectLinkedCollision() const { return *(Entity*)(ent + 0x204); }
	inline int pitch() const { return *(int*)(ent + 0x258); }
	inline int hitboxOffsetX() const { return *(int*)(ent + 0x27c); }
	inline int hitboxOffsetY() const { return *(int*)(ent + 0x280); }
	inline int hitboxCount(HitboxType type) const { return *(int*)(ent + 0xa0 + (int)type * 4); }
	inline const Hitbox* hitboxData(HitboxType type) const { return *(const Hitbox**)(ent + 0x58 + (int)type * 4); }
	inline int hitAirPushbackX() const { return *(int*)(ent + 0x67c); }
	inline int hitAirPushbackY() const { return *(int*)(ent + 0x680); }
	inline int untechableTime() const { return *(int*)(ent + 0x694); }
	inline int floorBouncesRemaining() const { return *(int*)(ent + 0x960); }
	inline bool isOtg() const { return (*(DWORD*)(ent + 0x4d24) & 0x800000) != 0; }
	inline bool damageToAir() const { return (*(DWORD*)(ent + 0x4d24) & 0x8000) != 0; }  // this is present on Answer Backdash, Faust Pogo, May Horizontal Dolphin first few frames, etc
	inline bool crouching() const { return (*(DWORD*)(ent + 0x4d24) & 0x1) != 0; }
	inline bool setOnCmnActDownBoundEntry() const { return (*(DWORD*)(ent + 0x4d24) & 2) != 0; }  // this is set when entering CmnActDownBound animation. Also present in all of hitstun animations
	inline bool lying() const { return (*(DWORD*)(ent + 0x4d24) & 0x4) != 0; }
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
	inline int venomBallArg3() const { return *(int*)(ent + 0x25bc); }
	inline int createArgHikitsukiVal2_outgoing() const { return *(int*)(ent + 0x2614 + 0x38); }
	inline const char* previousAnimName() const { return (const char*)(ent + 0x2424); }
	inline int groundHitEffect() const { return *(int*)(ent + 0x6a4); }
	inline int groundBounceCount() const { return *(int*)(ent + 0x69c); }
	inline int tumbleDuration() const { return *(int*)(ent + 0x6b4); }  // dealt or prepared attack's tumble
	inline int knockdown() const { return *(int*)(ent + 0x964); }  // received knockdown duration maximum, does not decrement over time
	inline int wallstick() const { return *(int*)(ent + 0x970); }  // received wallstick duration maximum, does not decrement over time
	inline int tumble() const { return *(int*)(ent + 0x978); }  // received tumble maximum, does not decrement over time
	inline int airDashMinimumHeight() const { return *(int*)(ent + 0x9864); }
	inline int framesSinceRegisteringForTheIdlingSignal() const { return *(int*)(ent + 0x140); }  // unfortunately, this value is +1 at the end of the tick more than what it was during an idling event. Luckily, you can just -1 from it before using
	inline int relatedToBufferTime1() const { return *(int*)(ent + 0x24dcc); }
	inline int relatedToBufferTime2() const { return *(int*)(ent + 0x24dd0); }
	inline int relatedToBufferTime3() const { return *(int*)(ent + 0x24dd4); }
	inline int toAddToBufferTime() const { return *(int*)(ent + 0x24dd8); }
	inline int ensureAtLeast3fBufferForNormalsWhenJumping() const { return *(int*)(ent + 0x262f4); }
	inline const ScheduledAnim* currentAnimData() const { return (ScheduledAnim*)(ent + 0x24c0); }
	inline const ScheduledAnim* nextAnim() const { return (ScheduledAnim*)(ent + 0x2474); }
	inline int TrainingEtc_ComboDamage() const { return *(int*)(ent + 0x9f44); }
	
	void getState(EntityState* state, bool* wasSuperArmorEnabled = nullptr, bool* wasFullInvul = nullptr) const;
	
	static void getWakeupTimings(CharacterType charType, WakeupTimings* output);
	void getWakeupTimings(WakeupTimings* output) const;
	int calculateGuts(int* gutsLevel = nullptr) const;
	
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
	getPushbox_t getPushboxWidth;
	getPushbox_t getPushboxTop;
	getPushbox_t getPushboxBottom;
	getExtraTensionModifier_t getExtraTensionModifier = nullptr;
};

extern EntityManager entityManager;
