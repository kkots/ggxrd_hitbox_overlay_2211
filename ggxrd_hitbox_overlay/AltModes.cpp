#include "pch.h"
#include "AltModes.h"
#include "memoryFunctions.h"

AltModes altModes;

bool AltModes::onDllMain() {
	
	// ghidra sig: 8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 3c 10 75 16 8b 15 ?? ?? ?? ?? 39 b2 f0 00 00 00 74 08 c7 44 24 48 d9 00 00 00
	pauseMenu = (char*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x8b\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x3c\x10\x75\x16\x8b\x15\x00\x00\x00\x00\x39\xb2\xf0\x00\x00\x00\x74\x08\xc7\x44\x24\x48\xd9\x00\x00\x00",
		"xx????x????xxxxxx????xxxxxxxxxxxxxxxx",
		{17, 0},
		NULL, "pauseMenu");

	// ghidra sig: a1 ?? ?? ?? ?? b9 ?? ?? ?? ?? 89 47 28 e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 8b b1 ?? ?? ?? ?? a1 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 0f 57 c0 8b 1e 33 ed 55 8d 54 24 14
	isIKCutscenePlaying = (char*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\xa1\x00\x00\x00\x00\xb9\x00\x00\x00\x00\x89\x47\x28\xe8\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x00\x8b\xb1\x00\x00\x00\x00\xa1\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x01\x0f\x57\xc0\x8b\x1e\x33\xed\x55\x8d\x54\x24\x14",
		"x????x????xxxx????xx????xx????x????xx????xxxxxxxxxxxx",
		{1, 0},
		NULL, "isIKCutscenePlaying");

	if (isIKCutscenePlaying) roundendCameraFlybyTypeRef = isIKCutscenePlaying - 16;

	// ghidra sig: e8 ?? ?? ?? ?? 03 f3 83 fe 66 7c e5 a1 ?? ?? ?? ?? 39 3d ?? ?? ?? ?? 74 0f 29 1d ?? ?? ?? ?? 83 f8 05 73 13 03 c3 eb 0a 3b c7 0f 84 99 00 00 00 2b c3
	versusModeMenuOpenRef = (char*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\xe8\x00\x00\x00\x00\x03\xf3\x83\xfe\x66\x7c\xe5\xa1\x00\x00\x00\x01\x39\x3d\x00\x00\x00\x01\x74\x0f\x29\x1d\x00\x00\x00\x01\x83\xf8\x05\x73\x13\x03\xc3\xeb\x0a\x3b\xc7\x0f\x84\x99\x00\x00\x00\x2b\xc3",
		"x????xxxxxxxx????xx????xxxx????xxxxxxxxxxxxxxxxxxx",
		{ 19, 0 },
		NULL, "versusModeMenuOpenRef");

	return true;
}

bool AltModes::isGameInNormalMode(bool* needToClearHitDetection) {
	if (isIKCutscenePlaying && *isIKCutscenePlaying) {
		if (needToClearHitDetection) *needToClearHitDetection = true;
		return false;
	}
	if (pauseMenu && *(unsigned int*)(pauseMenu + 0xFC) == 1 && (*(unsigned int*)(pauseMenu + 0x10) & 0x10000) == 0
		|| versusModeMenuOpenRef && *versusModeMenuOpenRef) {
		return false;
	}
	return true;
}

char AltModes::roundendCameraFlybyType() const {
	if (roundendCameraFlybyTypeRef) return *roundendCameraFlybyTypeRef;
	return 8;
}
