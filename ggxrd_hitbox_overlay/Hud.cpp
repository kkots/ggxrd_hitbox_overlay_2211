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
	// ghidra sig: 83 3d ?? ?? ?? ?? 00 8b f1 c7 44 24 18 00 00 00 00 0f 84 d7 06 00 00 8b 86 d8 00 00 00 8b 88 a8 04 00 00 51 e8 ?? ?? ?? ?? 83 c4 04 89 44 24 14 85 c0 0f 84 b6 06 00 00
	orig_REDHUDBattleUpdateAll = (REDHUDBattleUpdateAll_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x83\x3d\x00\x00\x00\x00\x00\x8b\xf1\xc7\x44\x24\x18\x00\x00\x00\x00\x0f\x84\xd7\x06\x00\x00\x8b\x86\xd8\x00\x00\x00\x8b\x88\xa8\x04\x00\x00\x51\xe8\x00\x00\x00\x00\x83\xc4\x04\x89\x44\x24\x14\x85\xc0\x0f\x84\xb6\x06\x00\x00",
		"xx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxx",
		{-0x3B},
		nullptr, "REDHUDBattleUpdateAll");

	if (orig_REDHUDBattleUpdateAll) {

		void(HookHelp::* REDHUDBattleUpdateAllHookPtr)() = &HookHelp::REDHUDBattleUpdateAllHook;
		detouring.attach(&(PVOID&)(orig_REDHUDBattleUpdateAll),
			(PVOID&)REDHUDBattleUpdateAllHookPtr,
			&orig_REDHUDBattleUpdateAllMutex,
			"REDHUDBattleUpdateAll");
	}

	return !error;
}

void Hud::HookHelp::REDHUDBattleUpdateAllHook() {
	++detouring.hooksCounter;
	detouring.markHookRunning("REDHUDBattleUpdateAll", true);
	hud.REDHUDBattleUpdateAllHook((char*)this);
	detouring.markHookRunning("REDHUDBattleUpdateAll", false);
	--detouring.hooksCounter;
}

void Hud::REDHUDBattleUpdateAllHook(char* thisArg) {
	hudPtr = thisArg;
	if (aswEngine && *aswEngine && game.isTrainingMode()
			&& (gifMode.gifModeOn || gifMode.gifModeToggleHudOnly)) {
		changeHudVisibility(false);
	} else {
		changeHudVisibility(true);
	}
	std::unique_lock<std::mutex> guard(orig_REDHUDBattleUpdateAllMutex);
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
