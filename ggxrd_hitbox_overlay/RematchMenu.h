#pragma once
#include "pch.h"

// because of these pesky typedefs, you probably don't want to include this header file everywhere in your program
typedef DWORD undefined4;
typedef char undefined;
typedef short undefined2;
typedef void UTexture2D;

struct RematchMenu {
	void* vtable;  // second func is handleEvent, third should be getTotalItemsNum but it's hardcoded to return 4 in the rematch menu... so idk. Has 17 functions
	int menuIndex;  // allows to identify this menu
	int padID;  // my guess is in offline versus, padID is used to read inputs for the gamepad that corresponds to this menu. I think it gets compared to MainQuadrant to determine if its P1 or P2
	undefined4 field0xc;
	DWORD flags;
	undefined4 field0x14;
	int field0x18;
	undefined4 field0x1c;
	undefined4 field0x20;
	int field0x24;
	int field0x28;
	undefined4 field0x2c;
	char name[32];  // allows to identify this menu
	char field0x50[32];  // this string was observed to be always empty
	int field0x70;
	int field0x74;
	DWORD field0x78;
	BOOL noTimeoutTimer;  // could not understand when this is set to 1
	int timeoutTimer;  // the timeout timers have not been debugged by me at all so understanding is extremely poor
	char score;
	char maxScore;  // set to -1 to disable. Used in ranked mode and those center cabinets in world lobbies I guess
	undefined unknownStuff[54];  // might not be 1-byte elements or used as an actual array at all
	undefined4 field0xbc;
	undefined4 field0xc0;
	int currentSelectedItem;  // index of item, starts from 0
	undefined4 field0xc8;
	undefined4 field0xcc;
	int field0xd0;
	int alsoCurrentSelectedItem;
	undefined4 field0xd8;
	undefined4 field0xdc;
	undefined4 field0xe0;
	undefined4 field0xe4;
	undefined4 field0xe8;
	undefined4 field0xec;
	int shouldIncrementCounter100AndUnlimitedWhen1Or2;
	undefined4 field0xf4;
	int field0xf8;
	undefined4 field0xfc;
	undefined4 field0x100;
	int field0x104;
	int counterUnlimited;  // counts up when shouldIncrementCounter100AndUnlimitedWhen1Or2 is 1 or 2
	undefined4 field0x10c;
	struct MenuDirectionsController {
		enum MenuDirection {
			MENU_DIRECTION_UP,
			MENU_DIRECTION_DOWN,
			MENU_DIRECTION_LEFT,
			MENU_DIRECTION_RIGHT,
			MENU_DIRECTION_NONE = -1
		} direction;
		int holdDuration;
		BOOL mustReleaseAll;
		int timerLeftRight;  // measures for how long left or right has been held
		int timerUpDown;
	} dirCtrl;
	int counter100_1;  // counts up to 100 when shouldIncrementCounter100AndUnlimitedWhen1Or2 is 1 or 2
	int counter100_2;
	undefined4 field0x12c;
	undefined4 field0x130;
	undefined4 field0x134;
	undefined4 field0x138;
	int field0x13c;
	undefined field_0x140[4];
	int field0x144;
	undefined field_0x148[4];
	int field0x14c;
	undefined field_0x150[4];
	undefined4 field0x154;
	DWORD field0x158;
	undefined2 field0x15c;
	undefined field_0x15e[2];
	undefined field0x160[8];
	BOOL isNetwork;
	BOOL isDirectParticipant;  // only makes sense when isNetwork
	int playerIndex;  // 0 or 1, never anything else. Does not make sense when isNetwork && !isDirectParticipant
	int rematchMenuIndex;  // must match playerIndex to be able to control
	int timeoutTimerPre;
	UTexture2D* scoreBackground;
	UTexture2D* coloredStarTex;
	UTexture2D* uncoloredStarTex;
};
