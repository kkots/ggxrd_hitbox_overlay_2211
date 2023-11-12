#include "pch.h"
#include "Hud.h"
#include "Detouring.h"
#include "memoryFunctions.h"
#include "GifMode.h"
#include "Game.h"

Hud hud;

bool Hud::onDllMain() {
	bool error = false;

	orig_updateHud = (updateHud_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x83\x3d\x00\x00\x00\x00\x00\x8b\xf1\xc7\x44\x24\x18\x00\x00\x00\x00\x0f\x84\xd7\x06\x00\x00\x8b\x86\xd8\x00\x00\x00\x8b\x88\xa8\x04\x00\x00\x51\xe8\x00\x00\x00\x00\x83\xc4\x04\x89\x44\x24\x14\x85\xc0\x0f\x84\xb6\x06\x00\x00",
		"xx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxx",
		{-0x3B},
		nullptr, "updateHud");

	if (orig_updateHud) {

		void(HookHelp::*updateHudHookPtr)() = &HookHelp::updateHudHook;
		detouring.attach(&(PVOID&)(orig_updateHud),
			(PVOID&)updateHudHookPtr,
			&orig_updateHudMutex,
			"updateHud");
	}

	return !error;
}

void Hud::HookHelp::updateHudHook() {
	++detouring.hooksCounter;
	detouring.markHookRunning("updateHud", true);
	hud.updateHudHook((char*)this);
	detouring.markHookRunning("updateHud", false);
	--detouring.hooksCounter;
}

void Hud::updateHudHook(char* thisArg) {
	hudPtr = thisArg;
	if (aswEngine && *aswEngine && game.isTrainingMode() && gifMode.gifModeOn) {
		changeHudVisibility(false);
	} else {
		changeHudVisibility(true);
	}
	std::unique_lock<std::mutex> guard(orig_updateHudMutex);
	orig_updateHud(thisArg);
}

void Hud::changeHudVisibility(bool isVisible) {
	if (!hudPtr) return;
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
