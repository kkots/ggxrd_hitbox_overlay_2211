#include "pch.h"
#include "InputNames.h"

std::vector<InputName> inputNames{ 0x127 };

void fillInInputNames() {
	inputNames[/*0x1*/INPUT_HOLD_P] = { "[P]", BUTTON, -1 };
	inputNames[/*0x2*/INPUT_P_STRICT_PRESS] = { "S (no buffer)",  BUTTON, 1 };
	inputNames[/*0x3*/INPUT_P_STRICT_RELEASE] = { "]S[ (no buffer)",  BUTTON, 1 };
	inputNames[/*0x4*/INPUT_PRESS_P] = { "P",  BUTTON, 3 };
	inputNames[/*0x5*/INPUT_NOT_HOLD_P] = { "don't hold P",  MULTIWORD_BUTTON, -1 };
	inputNames[/*0x6*/INPUT_RELEASE_P] = { "]P[",  BUTTON, 3 };
	inputNames[/*0xa*/INPUT_HOLD_K] = { "[K]", BUTTON, -1 };
	inputNames[/*0xb*/INPUT_K_STRICT_PRESS] = { "K (no buffer)", BUTTON, 1 };
	inputNames[/*0xc*/INPUT_K_STRICT_RELEASE] = { "]K[ (no buffer)", BUTTON, 1 };
	inputNames[/*0xd*/INPUT_PRESS_K] = { "K", BUTTON, 3 };
	inputNames[/*0xe*/INPUT_NOT_HOLD_K] = { "don't hold K", MULTIWORD_BUTTON, -1 };
	inputNames[/*0xf*/INPUT_RELEASE_K] = { "]K[", BUTTON, 3 };
	inputNames[/*0x13*/INPUT_HOLD_S] = { "[S]", BUTTON, -1 };
	inputNames[/*0x14*/INPUT_S_STRICT_PRESS] = { "S (no buffer)", BUTTON, 1 };
	inputNames[/*0x15*/INPUT_S_STRICT_RELEASE] = { "]S[ (no buffer)", BUTTON, 1 };
	inputNames[/*0x16*/INPUT_PRESS_S] = { "S", BUTTON, 3 };
	inputNames[/*0x17*/INPUT_NOT_HOLD_S] = { "don't hold S", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x18*/INPUT_RELEASE_S] = { "]S[", BUTTON, 3 };
	inputNames[/*0x1c*/INPUT_HOLD_H] = { "[H]", BUTTON, -1 };
	inputNames[/*0x1d*/INPUT_H_STRICT_PRESS] = { "H (no buffer)", BUTTON, 1 };
	inputNames[/*0x1e*/INPUT_H_STRICT_RELEASE] = { "]H[ (no buffer)", BUTTON, 1 };
	inputNames[/*0x1f*/INPUT_PRESS_H] = { "H", BUTTON, 3 };
	inputNames[/*0x20*/INPUT_NOT_HOLD_H] = { "don't hold H", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x21*/INPUT_RELEASE_H] = { "]H[", BUTTON, 3 };
	inputNames[/*0x25*/INPUT_HOLD_D] = { "[D]", BUTTON, -1 };
	inputNames[/*0x26*/INPUT_D_STRICT_PRESS] = { "D (no buffer)", BUTTON, 1 };
	inputNames[/*0x27*/INPUT_D_STRICT_RELEASE] = { "]D[ (no buffer)", BUTTON, 1 };
	inputNames[/*0x28*/INPUT_PRESS_D] = { "D", BUTTON, 3 };
	inputNames[/*0x29*/INPUT_NOT_HOLD_D] = { "don't hold D", MULTIWORD_BUTTON, 1 };
	inputNames[/*0x2a*/INPUT_RELEASE_D] = { "]D[", BUTTON, 3 };
	inputNames[/*0x2e*/INPUT_HOLD_TAUNT] = { "[Taunt]", BUTTON, -1 };
	inputNames[/*0x2f*/INPUT_TAUNT_STRICT_PRESS] = { "Taunt (no buffer)", BUTTON, 1 };
	inputNames[/*0x30*/INPUT_TAUNT_STRICT_RELEASE] = { "]Taunt[ (no buffer)", BUTTON, 1 };
	inputNames[/*0x31*/INPUT_PRESS_TAUNT] = { "Taunt", BUTTON, 3 };
	inputNames[/*0x32*/INPUT_NOT_HOLD_TAUNT] = { "don't hold Taunt", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x33*/INPUT_RELEASE_TAUNT] = { "Taunt", BUTTON, 3 };
	inputNames[/*0x37*/INPUT_1] = { "1", MOTION, -1 };
	inputNames[/*0x38*/INPUT_4_OR_1_OR_2] = { "4/1/2", MOTION, -1 };
	inputNames[/*0x3f*/INPUT_NOT_1] = { "don't hold 1", MULTIWORD_MOTION, -1 };
	inputNames[/*0x40*/INPUT_NOT_4_OR_1_OR_2] = { "don't hold 4/1/2", MULTIWORD_MOTION, -1 };
	inputNames[/*0x44*/INPUT_2] = { "2", MOTION, -1 };
	inputNames[/*0x45*/INPUT_ANYDOWN] = { "1/2/3", MOTION, -1 };
	inputNames[/*0x47*/INPUT_ANYDOWN_STRICT_PRESS] = { "press 1/2/3 (no buffer)", MULTIWORD_BUTTON, 1 };
	inputNames[/*0x4c*/INPUT_NOT_2] = { "don't hold 2", MULTIWORD_MOTION, -1 };
	inputNames[/*0x4d*/INPUT_NOTANYDOWN] = { "don't hold 1/2/3", MULTIWORD_MOTION, -1 };
	inputNames[/*0x51*/INPUT_3] = { "3", MOTION, -1 };
	inputNames[/*0x52*/INPUT_6_OR_3_OR_2] = { "6/3/2", MOTION, -1 };
	inputNames[/*0x59*/INPUT_NOT_3] = { "don't hold 3", MULTIWORD_MOTION, -1 };
	inputNames[/*0x5a*/INPUT_NOT_6_OR_3_OR_2] = { "don't hold 6/3/2", MULTIWORD_MOTION, -1 };
	inputNames[/*0x5e*/INPUT_4] = { "4", MOTION, -1 };
	inputNames[/*0x5f*/INPUT_ANYBACK] = { "7/4/1", MOTION, -1 };
	inputNames[/*0x61*/INPUT_ANYBACK_STRICT_PRESS] = { "press 7/4/1 (no buffer)", MULTIWORD_BUTTON, 1 };
	inputNames[/*0x66*/INPUT_NOT_4] = { "don't hold 4", MULTIWORD_MOTION, -1 };
	inputNames[/*0x67*/INPUT_NOTANYBACK] = { "don't hold 7/4/1", MULTIWORD_MOTION, -1 };
	inputNames[/*0x6b*/INPUT_5] = { "5", MOTION, -1 };
	inputNames[/*0x6c*/INPUT_ALWAYS_TRUE_DUPLICATE] = { "nullptr", MULTIWORD_MOTION, -1 };
	inputNames[/*0x73*/INPUT_NOT_5] = { "don't hold 5", MULTIWORD_MOTION, -1 };
	inputNames[/*0x78*/INPUT_6] = { "6", MOTION, -1 };
	inputNames[/*0x79*/INPUT_ANYFORWARD] = { "9/6/3", MOTION, -1 };
	inputNames[/*0x7b*/INPUT_ANYFORWARD_STRICT_PRESS] = { "press 9/6/3 (no buffer)", MULTIWORD_BUTTON, 1 };
	inputNames[/*0x80*/INPUT_NOT_6] = { "don't hold 6", MULTIWORD_MOTION, -1 };
	inputNames[/*0x81*/INPUT_NOTANYFORWARD] = { "don't hold 9/6/3", MULTIWORD_MOTION, -1 };
	inputNames[/*0x85*/INPUT_7] = { "7", MOTION, -1 };
	inputNames[/*0x86*/INPUT_4_OR_7_OR_8] = { "4/7/8", MOTION, -1 };
	inputNames[/*0x8d*/INPUT_NOT_7] = { "don't hold 7", MULTIWORD_MOTION, -1 };
	inputNames[/*0x8e*/INPUT_NOT_4_OR_7_OR_8] = { "don't hold 4/7/8", MULTIWORD_MOTION, -1 };
	inputNames[/*0x92*/INPUT_8] = { "8", MOTION, -1 };
	inputNames[/*0x93*/INPUT_ANYUP] = { "7/8/9", MOTION, -1 };
	inputNames[/*0x95*/INPUT_ANYUP_STRICT_PRESS] = { "press 7/8/9 (no buffer)", MULTIWORD_BUTTON, 1 };
	inputNames[/*0x9a*/INPUT_NOT_8] = { "don't hold 8", MULTIWORD_MOTION, -1 };
	inputNames[/*0x9b*/INPUT_NOTANYUP] = { "don't hold 7/8/9", MULTIWORD_MOTION, -1 };
	inputNames[/*0x9f*/INPUT_9] = { "9", MOTION, -1 };
	inputNames[/*0xa0*/INPUT_6_OR_9_OR_8] = { "6/9/8", MOTION, -1 };
	inputNames[/*0xa7*/INPUT_NOT_9] = { "don't hold 9", MULTIWORD_MOTION, -1 };
	inputNames[/*0xa8*/INPUT_NOT_6_OR_9_OR_8] = { "don't hold 6/9/8", MULTIWORD_MOTION, -1 };
	inputNames[/*0xac*/INPUT_236] = { "236", MOTION, 10 };
	inputNames[/*0xad*/INPUT_623] = { "623", MOTION, 7 };
	inputNames[/*0xae*/INPUT_214] = { "214", MOTION, 10 };
	inputNames[/*0xaf*/INPUT_41236] = { "41236", MOTION, 12 };
	inputNames[/*0xb0*/INPUT_421] = { "421", MOTION, 7 };
	inputNames[/*0xb1*/INPUT_63214] = { "63214", MOTION, 12 };
	inputNames[/*0xb2*/INPUT_236236] = { "236236", MOTION, 12 };
	inputNames[/*0xb3*/INPUT_214214] = { "214214", MOTION, 12 };
	inputNames[/*0xb4*/INPUT_4123641236] = { "4123641236", MOTION, 10 };
	inputNames[/*0xb5*/INPUT_6321463214] = { "6321463214", MOTION, 10 };
	inputNames[/*0xb6*/INPUT_632146] = { "632146", MOTION, 8 };  // some versions of this motion have a 12f buffer
	inputNames[/*0xb7*/INPUT_641236] = { "641236", MOTION, 14 };
	inputNames[/*0xb8*/INPUT_2141236] = { "2141236", MOTION, 10 };
	inputNames[/*0xb9*/INPUT_2363214] = { "2363214", MOTION, 10 };
	inputNames[/*0xba*/INPUT_22] = { "22", MOTION, 10 };
	inputNames[/*0xbb*/INPUT_46] = { "46", MOTION, 10 };
	inputNames[/*0xbc*/INPUT_CHARGE_BACK_FORWARD_30F] = { "charge back 30f -> forward", MULTIWORD_MOTION, 10 };
	inputNames[/*0xbd*/INPUT_CHARGE_DOWN_UP_30F] = { "charge down 30f -> up", MULTIWORD_MOTION, 10 };
	inputNames[/*0xbe*/INPUT_6428] = { "6428", MOTION, 10 };
	inputNames[/*0xbf*/INPUT_CHARGE_BACK_UP_30F] = { "charge back 30f -> up", MULTIWORD_MOTION, 10 };
	inputNames[/*0xc0*/INPUT_64641236] = { "64641236", MOTION, 10 };
	inputNames[/*0xc1*/INPUT_342646] = { "342646", MOTION, 10 };
	inputNames[/*0xc2*/INPUT_28] = { "28", MOTION, 10 };
	inputNames[/*0xc3*/INPUT_646] = { "646", MOTION, 7 };
	inputNames[/*0xc4*/INPUT_P_MASH] = { "mash (press 5 times) P", MULTIWORD_BUTTON, 12 };
	inputNames[/*0xc5*/INPUT_K_MASH] = { "mash (press 5 times) K", MULTIWORD_BUTTON, 12 };
	inputNames[/*0xc6*/INPUT_S_MASH] = { "mash (press 5 times) S", MULTIWORD_BUTTON, 12 };
	inputNames[/*0xc7*/INPUT_H_MASH] = { "mash (press 5 times) H", MULTIWORD_BUTTON, 12 };
	inputNames[/*0xc8*/INPUT_D_MASH] = { "mash (press 5 times) D", MULTIWORD_BUTTON, 12 };
	inputNames[/*0xc9*/INPUT_CIRCLE] = { "circle", MULTIWORD_MOTION, 30 };  // I don't have the energy or desire to understand what the actual buffer window on this is
	inputNames[/*0xcc*/INPUT_222] = { "222", MOTION, 8 };
	inputNames[/*0xcd*/INPUT_2222] = { "2222", MOTION, 10 };
	inputNames[/*0xce*/INPUT_236236_STRICTER] = { "236236", MOTION, 10 };
	inputNames[/*0xd4*/INPUT_PRESS_D_DUPLICATE] = { "D", BUTTON, 3 };
	inputNames[/*0xd5*/INPUT_HOLD_6_OR_3_AND_PRESS_TWO_OF_PKSH] = { "6/3+any two of PKSH", MULTIWORD_BUTTON, 3 };
	inputNames[/*0xd6*/INPUT_ROMAN_CANCEL] = { "Roman Cancel", MULTIWORD_BUTTON, 3 };
	inputNames[/*0xd7*/INPUT_SUPERJUMP] = { "Superjump", BUTTON, 1 };
	inputNames[/*0xd8*/INPUT_ANYUP_STRICT_PRESS_DUPLICATE] = { "press 7/8/9 (no buffer)", MULTIWORD_BUTTON, 1 };
	inputNames[/*0xd9*/INPUT_FORWARD_DASH] = { "66", MOTION, 3 };
	inputNames[/*0xda*/INPUT_BACKDASH] = { "44", MOTION, 2 };
	inputNames[/*0xdb*/INPUT_BLOCK_WITH_CROSSUP_PROTECTION] = { "hold block (when crossup immunity (3f) is in effect, both directions of block will suffice or even not blocking"
		" at all and simply having held block within the past 6f."
		" If in the air and the opponent is behind you, you may hold block in either direction)", MULTIWORD_MOTION, -1 };
	inputNames[/*0xdc*/INPUT_BLOCK_OR_CROSSUP_AIR_BLOCK] = { "hold block (or if in the air and the opponent is behind you, then you may hold block in either direction)", MULTIWORD_MOTION, -1 };
	inputNames[/*0xdd*/INPUT_151] = { "151", MOTION, 6 };
	inputNames[/*0xde*/INPUT_252] = { "252", MOTION, 6 };
	inputNames[/*0xdf*/INPUT_353] = { "353", MOTION, 6 };
	inputNames[/*0xe0*/INPUT_454] = { "454", MOTION, 6 };
	inputNames[/*0xe1*/INPUT_66_QUICK] = { "66", MOTION, 6 };
	inputNames[/*0xe2*/INPUT_757] = { "757", MOTION, 6 };
	inputNames[/*0xe3*/INPUT_858] = { "858", MOTION, 6 };
	inputNames[/*0xe4*/INPUT_959] = { "959", MOTION, 6 };
	inputNames[/*0xe9*/INPUT_ALWAYS_TRUE_DUPLICATE2] = { "nullptr", MULTIWORD_MOTION, -1 };
	inputNames[/*0xea*/INPUT_ALWAYS_FALSE_DUPLICATE] = { "nullptr", MULTIWORD_MOTION, -1 };
	inputNames[/*0xeb*/INPUT_623_LENIENT] = { "623", MOTION, 14 };
	inputNames[/*0xee*/INPUT_22_LENIENT] = { "22", MOTION, 14 };
	inputNames[/*0xef*/INPUT_5_OR_4_OR_ANY_UP] = { "5/4/7/8/9", MOTION, -1 };
	inputNames[/*0xf0*/INPUT_5_OR_ANY_UP] = { "5/7/8/9", MOTION, -1 };
	inputNames[/*0xf1*/INPUT_5_OR_6_OR_ANY_UP] = { "5/6/7/8/9", MOTION, -1 };
	inputNames[/*0xf2*/INPUT_5_OR_4_OR_7_OR_8] = { "5/4/7/8", MOTION, -1 };
	inputNames[/*0xf3*/INPUT_421_LENIENT] = { "421", MOTION, 14 };
	inputNames[/*0xf6*/INPUT_16243] = { "16243", MOTION, 16 };
	inputNames[/*0xf7*/INPUT_546] = { "546", MOTION, 3 };
	inputNames[/*0xf9*/INPUT_5_ANYBACK_ANYFORWARD_STRICTER] = { "5(7/4/1)(9/6/3)", MOTION, 6 };
	inputNames[/*0xfa*/INPUT_5_ANYFORWARD_ANYBACK] = { "5(9/6/3)(7/4/1)", MOTION, 6 };
	inputNames[/*0xfb*/INPUT_CHARGE_BACK_FORWARD_40F] = { "charge back 40f -> forward", MULTIWORD_MOTION, 10 };
	inputNames[/*0xfc*/INPUT_CHARGE_DOWN_UP_40F] = { "charge down 40f -> up", MULTIWORD_MOTION, 10 };
	inputNames[/*0xfd*/INPUT_CHARGE_BACK_FORWARD_45F] = { "charge back 45f -> forward", MULTIWORD_MOTION, 10 };
	inputNames[/*0xfe*/INPUT_CHARGE_DOWN_UP_45F] = { "charge down 45f -> up", MULTIWORD_MOTION, 10 };
	inputNames[/*0xff*/INPUT_236236236] = { "236236236", MOTION, 12 };
	inputNames[/*0x100*/INPUT_623_WITHIN_LAST_3F] = { "623 (3f buffer)", MULTIWORD_MOTION, 3 };
	inputNames[/*0x101*/INPUT_5_ANYBACK_ANYFORWARD_WITHIN_LAST_2F] = { "5(7/4/1)(9/6/3) (2f buffer)", MULTIWORD_MOTION, 2 };
	inputNames[/*0x102*/INPUT_NOTANYDOWN_2] = { "(not 1/2/3) -> 2", MULTIWORD_MOTION, 5 };
	inputNames[/*0x103*/INPUT_46_WITHIN_LAST_1F] = { "46 (no buffer)", MULTIWORD_MOTION, 1 };
	inputNames[/*0x104*/INPUT_CHARGE_DOWN_10F] = { "charge down 10f", MULTIWORD_MOTION, -1 };
	inputNames[/*0x105*/INPUT_546_BUTNOT_54_ANYDOWN_6] = { "(546, but not 54(1/2/3)6)", MOTION, 3 };
	inputNames[/*0x106*/INPUT_5_ANYBACK_ANYFORWARD_LENIENT] = { "5(7/4/1)(9/6/3)", MOTION, 8 };
	inputNames[/*0x10c*/INPUT_BURST] = { "Burst", BUTTON, 3 };
	inputNames[/*0x10d*/INPUT_HOLD_TWO_OR_MORE_OF_PKSH] = { "hold two or more of PKSH", MULTIWORD_BUTTON, 3 };
	inputNames[/*0x10e*/INPUT_PRESS_TWO_OR_MORE_OF_PKSH_GLITCHED] = { "press two or more of PKSH (obsolete)", MULTIWORD_BUTTON, 3 };  // why it's glitched? I think it relies on a 2f buttonpress buffer. Rn it's 3. So it gets its flag set on f1 of press and then on f2 it's unset and on f3 it's set again and then you need to press the button again to continue it switching on and off each frame. Activating twice from one press on frames 1 and 3 is a glitch thank you for reading this far I hope you're happy and all is well
	inputNames[/*0x10f*/INPUT_PRESS_ANYBACK_WITHIN_LAST_8F_NO_MASH_ALLOWED] = { "7/4/1 within last 8f no mash allowed", MULTIWORD_BUTTON, 8 };
	inputNames[/*0x110*/INPUT_P_OR_K_OR_S_OR_H] = { "P/K/S/H", BUTTON, 3 };
	inputNames[/*0x111*/INPUT_BOOLEAN_OR] = { "or", BUTTON, -1 };
	inputNames[/*0x112*/INPUT_HAS_PRECEDING_5] = { "preceding 5", MULTIWORD_MOTION, -1 };
	inputNames[/*0x113*/INPUT_ITEM] = { "Item", BUTTON, 3 };
	inputNames[/*0x114*/INPUT_HOLD_SPECIAL] = { "[Sp]", BUTTON, -1 };
	inputNames[/*0x115*/INPUT_SPECIAL_STRICT_PRESS] = { "Sp (no buffer)", BUTTON, 1 };
	inputNames[/*0x116*/INPUT_SPECIAL_STRICT_RELEASE] = { "]Sp[ (no buffer)", BUTTON, 1 };
	inputNames[/*0x117*/INPUT_PRESS_SPECIAL] = { "Sp", BUTTON, 3 };
	inputNames[/*0x118*/INPUT_NOT_HOLD_SPECIAL] = { "don't hold Sp", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x119*/INPUT_RELEASE_SPECIAL] = { "]Sp[", BUTTON, 3 };
	inputNames[/*0x11d*/INPUT_ANY_TWO_OF_PKSH] = { "any two of PKSH", MULTIWORD_BUTTON, 3 };
	inputNames[/*0x11e*/INPUT_ROMAN_CANCEL_DUPLICATE] = { "Roman Cancel", MULTIWORD_BUTTON, 3 };
	inputNames[/*0x11f*/INPUT_MOM_TAUNT] = { "MOM Taunt", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x120*/INPUT_FORWARD_DASH_WITHIN_LAST_2F] = { "66 (2f buffer)", MULTIWORD_MOTION, 2 };
	inputNames[/*0x121*/INPUT_BACKDASH_WITHIN_LAST_2F] = { "44 (2f buffer)", MULTIWORD_MOTION, 2 };
	inputNames[/*0x122*/INPUT_P_OR_K_OR_S_OR_H_OR_D_STRICT_PRESS] = { "P/K/S/H/D (no buffer)", BUTTON, 1 };
	inputNames[/*0x123*/INPUT_ALWAYS_FALSE] = { "nullptr", MULTIWORD_MOTION, -1 };
	inputNames[/*0x124*/INPUT_PRESS_TAUNT_DUPLICATE] = { "Taunt", BUTTON, 3 };
	inputNames[/*0x125*/INPUT_HOLD_ONE_OR_MORE_OF_PKSH] = { "hold one or more of P/K/S/H", MULTIWORD_BUTTON, -1 };
	inputNames[/*0x126*/INPUT_HOLD_ONE_OR_MORE_OF_PKSH_OR_D] = { "hold one or more of P/K/S/H/D", MULTIWORD_BUTTON, -1 };
}
