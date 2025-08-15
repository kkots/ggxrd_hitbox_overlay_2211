#include "pch.h"
#include "AltModes.h"
#include "memoryFunctions.h"

AltModes altModes;

bool AltModes::onDllMain() {
	
	pauseMenu = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 3c 10 75 16 8b 15 ?? ?? ?? ?? 39 b2 f0 00 00 00 74 08 c7 44 24 48 d9 00 00 00",
		{17, 0},
		NULL, "pauseMenu");

	isIKCutscenePlaying = (char*)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"a1 ?? ?? ?? ?? b9 ?? ?? ?? ?? 89 47 28 e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 8b b1 ?? ?? ?? ?? a1 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 0f 57 c0 8b 1e 33 ed 55 8d 54 24 14",
		{1, 0},
		NULL, "isIKCutscenePlaying");

	if (isIKCutscenePlaying) roundendCameraFlybyTypeRef = isIKCutscenePlaying - 16;

	versusModeMenuOpenRef = (char*)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"e8 ?? ?? ?? ?? 03 f3 83 fe 66 7c e5 a1 ?? ?? ?? ?? 39 3d ?? ?? ?? ?? 74 0f 29 1d ?? ?? ?? ?? 83 f8 05 73 13 03 c3 eb 0a 3b c7 0f 84 99 00 00 00 2b c3",
		{ 19, 0 },
		NULL, "versusModeMenuOpenRef");

	return true;
}

bool AltModes::isGameInNormalMode(bool* needToClearHitDetection, bool* isPauseMenu) {
	if (isPauseMenu) *isPauseMenu = false;
	if (isIKCutscenePlaying && *isIKCutscenePlaying) {
		if (needToClearHitDetection) *needToClearHitDetection = true;
		return false;
	}
	if (versusModeMenuOpenRef && *versusModeMenuOpenRef) {
		if (isPauseMenu) *isPauseMenu = true;
		return false;
	}
	return true;
}

char AltModes::roundendCameraFlybyType() const {
	if (roundendCameraFlybyTypeRef) return *roundendCameraFlybyTypeRef;
	return 8;
}
