#include "pch.h"
#include "Hud.h"
#include "Detouring.h"
#include "memoryFunctions.h"
#include "GifMode.h"
#include "Game.h"

Hud hud;

bool Hud::onDllMain() {
	bool error = false;

	// REDHUDBattleUpdateAll is AREDHUD_Battle::UpdateAll
	orig_REDHUDBattleUpdateAll = (REDHUDBattleUpdateAll_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 3d ?? ?? ?? ?? 00 8b f1 c7 44 24 18 00 00 00 00 0f 84 d7 06 00 00 8b 86 d8 00 00 00 8b 88 a8 04 00 00 51 e8 ?? ?? ?? ?? 83 c4 04 89 44 24 14 85 c0 0f 84 b6 06 00 00",
		{-0x3B},
		nullptr, "REDHUDBattleUpdateAll");

	if (orig_REDHUDBattleUpdateAll) {

		void(HookHelp::* REDHUDBattleUpdateAllHookPtr)() = &HookHelp::REDHUDBattleUpdateAllHook;
		detouring.attach(&(PVOID&)(orig_REDHUDBattleUpdateAll),
			(PVOID&)REDHUDBattleUpdateAllHookPtr,
			"REDHUDBattleUpdateAll");
	}

	return !error;
}

void Hud::HookHelp::REDHUDBattleUpdateAllHook() {
	hud.REDHUDBattleUpdateAllHook((char*)this);
}

void Hud::REDHUDBattleUpdateAllHook(char* thisArg) {
	hudPtr = thisArg;
	if (aswEngine && *aswEngine && game.isTrainingMode()
			&& (gifMode.gifModeOn || gifMode.gifModeToggleHudOnly)) {
		changeHudVisibility(false);
	} else {
		changeHudVisibility(true);
	}
	orig_REDHUDBattleUpdateAll(thisArg);
}

void Hud::changeHudVisibility(bool isVisible) {
	if (!hudPtr || !aswEngine || !*aswEngine) return;
	unsigned int* bShowHud = (unsigned int*)(hudPtr + 0x1D8);
	if (isVisible) {
		*bShowHud |= 2;
	} else {
		*bShowHud &= ~2;
	}
}

void Hud::onDllDetach() {
	changeHudVisibility(true);
}
