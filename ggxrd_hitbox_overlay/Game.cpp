#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "Settings.h"
#include "EndScene.h"
#include "Camera.h"
#include "GifMode.h"
#include "WError.h"
#include "EntityList.h"
#include "AltModes.h"
#include "HitDetector.h"
#include "RematchMenu.h"

char** aswEngine = nullptr;

Game game;

extern "C" void drawWinsHookAsm();  // defined in asmhooks.asm
extern "C" BOOL __cdecl drawWinsHook(RematchMenu* rematchMenu);  // defined here

bool Game::onDllMain() {
	bool error = false;

	aswEngine = (char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"85 C0 78 74 83 F8 01",
		{-4, 0},
		&error, "aswEngine");
	
	// gameDataPtr is of type REDGameCommon** when not dereferenced
	gameDataPtr = (char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"33 C0 38 41 44 0F 95 C0 C3 CC",
		{-4, 0},
		&error, "gameDataPtr");

	playerSideNetworkHolder = (char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 3c 10 75 10 a1 ?? ?? ?? ?? 85 c0 74 07 c6 80 0c 12 00 00 01 c3",
		{16, 0},
		NULL, "playerSideNetworkHolder");

	uintptr_t UWorld_TickCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"89 9e f4 04 00 00 8b 0d ?? ?? ?? ?? f3 0f 11 04 24 6a 02 e8 ?? ?? ?? ?? 33 ff 39 9e d8 04 00 00 7e 37",
		{ 19 },
		&error, "UWorld_TickCallPlace");
	if (UWorld_TickCallPlace) {
		orig_UWorld_Tick = (UWorld_Tick_t)followRelativeCall(UWorld_TickCallPlace);
	}
	
	if (orig_UWorld_Tick) {
		
		uintptr_t IsPausedCallPlace = sigscanForward((uintptr_t)orig_UWorld_Tick, "e8 ?? ?? ?? ?? 85 c0");
		if (IsPausedCallPlace) {
			orig_UWorld_IsPaused = (UWorld_IsPaused_t)followRelativeCall(IsPausedCallPlace);
		}
		
		void(HookHelp::*UWorld_TickHookPtr)(ELevelTick TickType, float DeltaSeconds) = &HookHelp::UWorld_TickHook;
		detouring.attach(&(PVOID&)(orig_UWorld_Tick),
			(PVOID&)UWorld_TickHookPtr,
			"UWorld_Tick");
	}
	
	if (orig_UWorld_IsPaused) {
		bool(HookHelp::*UWorld_IsPausedHookPtr)() = &HookHelp::UWorld_IsPausedHook;
		detouring.attach(&(PVOID&)(orig_UWorld_IsPaused),
			(PVOID&)UWorld_IsPausedHookPtr,
			"UWorld_IsPaused");
	}
	
	if (!error && sigscanFrameByFraming()) {
		hookFrameByFraming();
	}
	
	if (aswEngine) {
		std::vector<char> burstSig;
		std::vector<char> burstMask;
		byteSpecificationToSigMask("8b 56 40 a1 ?? ?? ?? ?? 8b 84 90 ?? ?? ?? ?? 5f 5e 5d 5b 8b 4c 24 18 33 cc",
			burstSig, burstMask);
		substituteWildcard(burstSig, burstMask, 0, aswEngine);
		burstOffset = sigscanOffset(
			"GuiltyGearXrd.exe",
			burstSig,
			burstMask,
			{ 11, 0 },
			NULL, "burstOffset");
		
		std::vector<char> destroySig;
		std::vector<char> destroyMask;
		byteSpecificationToSigMask("c7 05 ?? ?? ?? ?? 00 00 00 00",
			destroySig, destroyMask);
		substituteWildcard(destroySig, destroyMask, 0, aswEngine);
		orig_destroyAswEngine = (destroyAswEngine_t)sigscanOffset(
			"GuiltyGearXrd.exe",
			destroySig,
			destroyMask,
			{ -0x5A },
			NULL, "destroyAswEngine");
	
		// offset from aswEngine to its field containing a pointer to an instance of AREDCamera_Battle class
		std::vector<char> cameraOffsetSig;
		std::vector<char> cameraOffsetSigMask;
		byteSpecificationToSigMask("8b 4c 24 18 83 c4 08 0b 4c 24 14 74 1e 8b 15 ?? ?? ?? ?? 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 75 09 55 e8 ?? ?? ?? ?? 83 c4 04",
			cameraOffsetSig, cameraOffsetSigMask);
	
		substituteWildcard(cameraOffsetSig, cameraOffsetSigMask, 0, aswEngine);
		// pointer to REDCamera_Battle
		cameraOffset = (unsigned int)sigscanOffset(
			"GuiltyGearXrd.exe",
			cameraOffsetSig,
			cameraOffsetSigMask,
			{ 0x15, 0 },
			&error, "cameraOffset");
		
	}
	
	if (cameraOffset) {
		REDGameInfo_BattleOffset = cameraOffset + 4;
		REDHUD_BattleOffset = cameraOffset - 4;
	}
	
	if (orig_destroyAswEngine) {
		detouring.attach(&(PVOID&)(orig_destroyAswEngine),
			destroyAswEngineHook,
			"destroyAswEngine");
	}
	
	aswEngineTickCountOffset = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8d bd a4 04 00 00 bd 02 00 00 00 90 8b 0f 3b cb 74 05 e8 ?? ?? ?? ?? 83 c7 04 4d 75 ef ff 86",
		{ 31, 0 },
		&error, "aswEngineTickCountOffset");
	if (aswEngineTickCountOffset) {
		dangerTimeOffset = aswEngineTickCountOffset - 0x2dc;
		matchInfoOffset = aswEngineTickCountOffset + 0x284;
	}
	

	std::vector<char> sig;
	std::vector<char> mask;
	if (REDHUD_BattleOffset) {
		byteSpecificationToSigMask("8b 81 ?? ?? ?? ?? f6 80 5c 04 00 00 08 74 14 8b 54 24 14 f6 82 c8 04 00 00 02",
			sig, mask);
		substituteWildcard(sig, mask, 0, (void*)REDHUD_BattleOffset);
		uintptr_t drawExGaugeHUDCallPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			nullptr, "drawExGaugeHUDCallPlace");
		if (drawExGaugeHUDCallPlace) {
			drawExGaugeHUDOffset = *(DWORD*)(drawExGaugeHUDCallPlace + 40);
			drawExGaugeHUD = (drawExGaugeHUD_t)followRelativeCall(drawExGaugeHUDCallPlace + 44);
			orig_drawExGaugeHUD = drawExGaugeHUD;
			
			void(HookHelp::*drawExGaugeHUDHookPtr)(int param_1) = &HookHelp::drawExGaugeHUDHook;
			detouring.attach(&(PVOID&)(orig_drawExGaugeHUD),
				(PVOID&)drawExGaugeHUDHookPtr,
				"drawExGaugeHUD");
		}
	}
	
	char _JitabataRecoverSig[] = "\0_JitabataRecover";
	
	uintptr_t _JitabataRecover = sigscanBufOffset(
		"GuiltyGearXrd.exe:.rdata",
		_JitabataRecoverSig,
		sizeof _JitabataRecoverSig,
		{ 1 },
		nullptr, "_JitabataRecover");
	
	uintptr_t stunmashDrawingPlace = 0;
	
	if (_JitabataRecover) {
		byteSpecificationToSigMask("53 53 53 68 ?? ?? ?? ?? 6a 15",
			sig, mask);
		substituteWildcard(sig, mask, 0, (void*)_JitabataRecover);
		
		stunmashDrawingPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig,
			mask,
			{ 0x2b2 },
			nullptr, "stunmashDrawingPlace");
		
	}
	
	if (stunmashDrawingPlace) {
		drawStunMashPtr = (drawStunMash_t)followRelativeCall(stunmashDrawingPlace);
	}
	
	// A nice way to find this is to search for a UTF16 string L"Steam must be running to play this game (SteamAPI_Init() failed)"
	// It will be near the top of the function, initialized this way:
	// if (local_1c == (undefined4 *)0x0) {
	//   DAT_0209cfa0 = (int *)0x0;
	// }
	// else {
	//   DAT_0209cfa0 = (int *)FUN_00fa1fa0();
	// }
	inputsHolder = (BYTE**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"e8 ?? ?? ?? ?? bb 00 00 01 00 bf 00 00 80 00",
		{ -12, 0 },
		nullptr, "inputsHolderCallPlace");
	
	drawJackoHouseHp = (drawJackoHouseHp_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"74 19 8b 86 40 25 00 00 39 86 3c 25 00 00 7d 0b",
		{ -0x26 },
		nullptr, "drawJackoHouseHp");
	
	roundendSuperfreezeCounterOffset = sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 bf ?? ?? ?? ?? 00 7e 31 33 d2 8b c2 3b 97 b0 00 00 00 7d 25 8b 84 87 f8 01 00 00 42 85 c0 74 19 8b 88 18 01 00 00 f6 c1 01 75 df 81 c9 00 00 00 04 89 88 18 01 00 00 eb d1",
		{ 2, 0 },
		nullptr, "roundendSuperfreezeCounter");
	if (roundendSuperfreezeCounterOffset) {
		aswEng0x1c710cOffset = roundendSuperfreezeCounterOffset + 0x35b4;  // probably better to make a dedicated sigscan, but we have too many already and it's slowing down the injection
	}
	
	postEffectOnPtr = (BOOL*)sigscanOffset(
		"GuiltyGearXrd.exe",
		"89 8e a4 01 00 00 8b 15 ?? ?? ?? ?? 89 96 a8 01 00 00 39 1d ?? ?? ?? ?? 0f 94 c0 33 c9 89 86 ac 01 00 00",
		{ 20, 0 },
		nullptr, "postEffectOn");
	
	orig_getRiscForUI_Background = (getRiscForUI_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"66 0f 6e 80 34 4e 02 00",
		{ -15 },
		nullptr, "getRiscForUIBackground");
	if (orig_getRiscForUI_Background) {
		float(HookHelp::*getRiscForUI_BackgroundHookPtr)() = &HookHelp::getRiscForUI_BackgroundHook;
		detouring.attach(&(PVOID&)(orig_getRiscForUI_Background),
			(PVOID&)getRiscForUI_BackgroundHookPtr,
			"getRiscForUIBackground");
		
		orig_getRiscForUI_Foreground = (getRiscForUI_t)((char*)orig_getRiscForUI_Background - 0x40);
	}
	
	if (orig_getRiscForUI_Foreground) {
		float(HookHelp::*getRiscForUI_ForegroundHookPtr)() = &HookHelp::getRiscForUI_ForegroundHook;
		detouring.attach(&(PVOID&)(orig_getRiscForUI_Foreground),
			(PVOID&)getRiscForUI_ForegroundHookPtr,
			"getRiscForUIForeground");
	}
	
	// Thanks to WorseThanYou for finding the location of the input ring buffers, their sizes and their structure
	inputRingBuffersOffset = (unsigned int)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 44 24 14 8b d7 6b d2 7e 56 8d 8c 02 ?? ?? ?? ?? e8 ?? ?? ?? ?? 66 89 74 7c 18",
		{ 13, 0 },
		nullptr, "inputRingBuffersOffset");
	
	// If WTY's position reset patch is applied, this function won't be found
	orig_setPositionResetType = (setPositionResetType_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"6a 02 56 e8 ?? ?? ?? ?? 8b c8 e8 ?? ?? ?? ?? 85 c0 74 13 e8 ?? ?? ?? ?? f7 d8 1b c0 f7 d8 40 a3 ?? ?? ?? ?? 5e c3 8b 0d ?? ?? ?? ?? 6a 08 56 e8",
		{ -14 },
		nullptr, "setPositionResetType");
	if (orig_setPositionResetType) {
		detouring.attach(&(PVOID&)(orig_setPositionResetType),
			setPositionResetTypeHookStatic,
			"setPositionResetType");
		
		getPlayerPadID = (getPlayerPadID_t)followRelativeCall((uintptr_t)orig_setPositionResetType + 1);
		
		uintptr_t roundInitUsage = sigscanOffset(
			"GuiltyGearXrd.exe",
			// c7 44 24 __ ________ is MOV [ESP+???], ???
			"c7 44 24 08 00 00 00 00 c7 44 24 0c 00 00 00 00 c7 44 24 10 d5 04 00 00 c7 44 24 14 b7 05 00 00 "
			"c7 44 24 18 d2 05 00 00 c7 44 24 1c 3b 06 00 00 c7 44 24 20 49 fa ff ff c7 44 24 24 2b fb ff ff "
			"c7 44 24 28 c5 f9 ff ff c7 44 24 2c 2e fa ff ff c7 44 24 30 98 ff ff ff c7 44 24 34 68 00 00 00 "
			"c7 44 24 38 e0 fc ff ff c7 44 24 3c 20 03 00 00 c7 44 24 40 ff ff ff ff c7 44 24 44 01 00 00 00",
			nullptr, "roundInitUsage");
		
		if (!roundInitUsage) return false;
		orig_roundInit = (roundInit_t)sigscanBackwards(roundInitUsage, "83 ec", 0x1f0);
		if (!orig_roundInit) return false;
		
		void(HookHelp::*roundInitHookPtr)() = &HookHelp::roundInitHook;
		detouring.attach(&(PVOID&)(orig_roundInit),
			(PVOID&)roundInitHookPtr,
			"roundInit");
	}
	
	uintptr_t placeInFunction0x10_OfNormalElementFor_AswSubEng_0x1c710c_0xac = sigscanOffset(
		"GuiltyGearXrd.exe",
		"e8 ?? ?? ?? ?? 8b d8 83 fb 1a",
		nullptr, "PlaceInFunction0x10_OfNormalElementFor_AswSubEng_0x1c710c_0xac");
	uintptr_t startOfFunction0x10 = 0;
	uintptr_t func0x10InRData = 0;
	if (placeInFunction0x10_OfNormalElementFor_AswSubEng_0x1c710c_0xac) {
		startOfFunction0x10 = sigscanBackwards(placeInFunction0x10_OfNormalElementFor_AswSubEng_0x1c710c_0xac, "83 ec", 0x20);  // SUB ESP,??
	}
	if (startOfFunction0x10) {
		func0x10InRData = sigscan(
			"GuiltyGearXrd.exe:.rdata",
			(const char*)&startOfFunction0x10, 4);
	}
	if (func0x10InRData) {
		normal0xa8ElementVtable = func0x10InRData - 0x10;
	}
	
	uintptr_t menuUsage = sigscanOffset("GuiltyGearXrd.exe",
		// use only a part of function to find it faster
		"83 b8 54 01 00 00 00 75 03 4e 79 eb",
		{ -15 },
		nullptr,
		"MenuUsage");
	if (menuUsage) {
		// validate the whole function, I'm going to need bits and pieces from it
		byteSpecificationToSigMask("56 be 65 00 00 00 56 e8 ?? ?? ?? ?? 83 c4 04 83 b8 54 01 00 00 00 75 03 4e 79 eb 89 35 ?? ?? ?? ?? 5e c3", sig, mask);
		if (sigscan(menuUsage, menuUsage + 0x23, sig.data(), mask.data()) != menuUsage) {
			menuUsage = 0;
		}
	}
	if (menuUsage) {
		currentMenu = *(int**)(menuUsage + 29);
		
		uintptr_t getMenu = followRelativeCall(menuUsage + 7);
		byteSpecificationToSigMask("8b 44 24 04 8b 04 85 ?? ?? ?? ?? c3", sig, mask);
		if (sigscan(getMenu, getMenu + 12, sig.data(), mask.data()) == getMenu) {
			allMenus = *(void***)(getMenu + 7);
		}
	}
	if (!currentMenu || !allMenus) {
		currentMenu = nullptr;
		allMenus = nullptr;
	}
	
	char RF_MenuStr[] = "RF_Menu";
	uintptr_t RF_MenuLoc = sigscan(
		"GuiltyGearXrd.exe:.rdata",
		RF_MenuStr,
		sizeof RF_MenuStr);
	
	uintptr_t RF_MenuUsage = 0;
	if (RF_MenuLoc) {
		char RF_DataUsageAr[5];
		RF_DataUsageAr[0] = 0x68;  // PUSH
		memcpy(RF_DataUsageAr + 1, &RF_MenuLoc, 4);
		RF_MenuUsage = sigscanOffset(
			"GuiltyGearXrd.exe",
			RF_DataUsageAr,
			"xxxxx",
			nullptr,
			"RF_MenuUsage");
	}
	uintptr_t case0x21 = 0;
	if (RF_MenuUsage) {
		byteSpecificationToSigMask("ff 24 85 ?? ?? ?? ?? 81 4b 10 04 81 00 00", sig, mask);
		if (sigscan(RF_MenuUsage - 14, RF_MenuUsage, sig.data(), mask.data()) == RF_MenuUsage - 14) {
			case0x21 = *(DWORD*)(7 * 4 + *(DWORD*)(RF_MenuUsage - 14 + 3));
		}
	}
	uintptr_t isGameModeNetworkCall = 0;
	if (case0x21) {
		byteSpecificationToSigMask("e8 ?? ?? ?? ?? 85 c0 0f 84 ?? ?? ?? ??", sig, mask);
		// calls isGameModeNetwork, tests EAX and jumps if zero
		isGameModeNetworkCall = sigscan(case0x21, case0x21 + 0x120, sig.data(), mask.data());
	}
	if (isGameModeNetworkCall) {
		isGameModeNetwork = (isGameModeNetwork_t)followRelativeCall(isGameModeNetworkCall);
		sig.resize(4);
		DWORD drawWinsHookAsmAddr = (DWORD)&drawWinsHookAsm;
		drawWinsHookAsmAddr = calculateRelativeCallOffset(isGameModeNetworkCall, drawWinsHookAsmAddr);
		memcpy(sig.data(), &drawWinsHookAsmAddr, 4);
		detouring.patchPlace(isGameModeNetworkCall + 1, sig);
	}
	
	return !error;
}

bool Game::sigscanAfterHitDetector() {
	uintptr_t getTrainingSettingCallPlace = 0;
	if (hitDetector.activeFrameHit) {
		getTrainingSettingCallPlace = sigscanForward(hitDetector.activeFrameHit,
			"6a 0b 89 6c 24 30 e8 ?? ?? ?? ?? 8b c8 e8 ?? ?? ?? ??",
			0xc00);
	}
	
	if (getTrainingSettingCallPlace) {
		getTrainingSettingPtr = (getTrainingSetting_t)followRelativeCall(getTrainingSettingCallPlace + 13);
	}
	
	if (hitDetector.orig_dealHit) {
		std::vector<char> sig;
		std::vector<char> mask;
		byteSpecificationToSigMask("e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ??", sig, mask);
		substituteWildcard(sig.data(), mask.data(), 2, aswEngine);
		uintptr_t isStylishCallPlace = sigscanForward((uintptr_t)hitDetector.orig_dealHit,
			sig.data(),
			mask.data(),
			0xf0);
		if (isStylishCallPlace) {
			isStylishPtr = (isStylish_t)followRelativeCall(isStylishCallPlace);
			stylishDefenseInverseModifierOffset = *(uintptr_t*)(isStylishCallPlace + 0x17);
			stylishBurstGainModifierOffset = stylishDefenseInverseModifierOffset + 8;
			handicapsOffset = *(uintptr_t*)(isStylishCallPlace + 0x23);
		}
	}
	cameraCenterXOffset = endScene.leftEdgeOfArenaOffset - 8;
	
	return true;
}

bool Game::sigscanFrameByFraming() {
	bool error = false;

	// TickActors<FDeferredTickList::FGlobalActorIterator>
	orig_TickActors_FDeferredTickList_FGlobalActorIterator = (TickActors_FDeferredTickList_FGlobalActorIterator_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 ec 18 53 55 57 8b 7c 24 28 8b 87 48 01 00 00 33 db 89 9f 44 01 00 00 3b c3 7d 22 8b 87 40 01 00 00 89 9f 48 01 00 00 3b c3 74 12 6a 08",
		&error, "TickActors_FDeferredTickList_FGlobalActorIterator");
	
	uintptr_t trainingHudCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"b9 ?? ?? ?? ?? e8 ?? ?? ?? ?? 84 c0 75 0c e8 ?? ?? ?? ?? 8b c8 e8 ?? ?? ?? ?? 8b 4c 24 1c",
		&error, "trainingHudCallPlace");
	
	if (trainingHudCallPlace) {
		getTrainingHud = (getTrainingHud_t)followRelativeCall(trainingHudCallPlace + 14);
		trainingHudTick = (trainingHudTick_t)followRelativeCall(trainingHudCallPlace + 21);
		// the hook for trainingHudTick is in EndScene
	}

	// updateBattleOfflineVer is the offline variant called from AREDGameInfo_Battle::UpdateBattle at E61240 (game version 2211, netcode version 2.15)
	// (it's in the non-else branch of the if ... == 0)
	std::vector<char> updateBattleOfflineVerSig;
	std::vector<char> updateBattleOfflineVerSigMask;
	byteSpecificationToSigMask("89 7c 24 14 e8 ?? ?? ?? ?? 85 c0 74 0a 6a 01 e8 ?? ?? ?? ?? 83 c4 04 8b 0d ?? ?? ?? ?? 8b 81 ?? ?? ?? ?? 8b 80 7c 03 00 00",
		updateBattleOfflineVerSig, updateBattleOfflineVerSigMask);
	substituteWildcard(updateBattleOfflineVerSig, updateBattleOfflineVerSigMask, 2, aswEngine);
	orig_updateBattleOfflineVer = (updateBattleOfflineVer_t)((sigscanOffset(
		"GuiltyGearXrd.exe",
		updateBattleOfflineVerSig,
		updateBattleOfflineVerSigMask,
		&error, "updateBattleOfflineVer") - 0x30) & 0xFFFFFFF0);
	logwrap(fprintf(logfile, "Final location of updateBattleOfflineVer: %p\n", orig_updateBattleOfflineVer));

	orig_TickActorComponents = (TickActorComponents_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"57 bf 01 00 00 00 39 7c 24 20 75 18 8b 06 8b 90 ec 01 00 00 8b ce ff d2 c7 44 24 10 00 00 00 00",
		{ -8 },
		&error, "TickActorComponents");

	return !error;
}

void Game::hookFrameByFraming() {
	
	detouring.attach(&(PVOID&)(orig_TickActors_FDeferredTickList_FGlobalActorIterator),
		Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookStatic,
		"TickActors_FDeferredTickList_FGlobalActorIterator");

	void(HookHelp::*updateBattleOfflineVerHookPtr)(int param1) = &HookHelp::updateBattleOfflineVerHook;
	detouring.attach(&(PVOID&)orig_updateBattleOfflineVer,
		(PVOID&)updateBattleOfflineVerHookPtr,
		"updateBattleOfflineVer");

	detouring.attach(&(PVOID&)(orig_TickActorComponents),
		Game::TickActorComponentsHookStatic,
		"TickActorComponents");

}

void Game::TickActorComponentsHookStatic(int param1, int param2, int param3, int param4) {
	game.TickActorComponentsHook(param1, param2, param3, param4);
}

void Game::TickActorComponentsHook(int param1, int param2, int param3, int param4) {
	if (!shutdown) {
		if (ignoreAllCalls) {
			return;
		}
	}
	orig_TickActorComponents(param1, param2, param3, param4);
}

void Game::HookHelp::updateBattleOfflineVerHook(int param1) {
	return game.updateBattleOfflineVerHook((char*)this, param1);
}

void Game::updateBattleOfflineVerHook(char* thisArg, int param1) {
	if (!shutdown && ignoreAllCalls) {
		return;
	}
	orig_updateBattleOfflineVer(thisArg, param1);
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookStatic(int param1, int param2, int param3, int param4) {
	game.TickActors_FDeferredTickList_FGlobalActorIteratorHook(param1, param2, param3, param4);
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHook(int param1, int param2, int param3, int param4) {
	// Approximate order in which things get called (when pause menu is not open and there's no rollback):
	// -1) ???
	// 0) UWorld::Tick is called, even when paused, in the game main (logic) thread
	// 1) This hook gets called in the game logic thread
	// 2) The hooked function calls updateBattleOfflineVerHook
	// 2.5) Inside updateBattleOfflineVerHook, drawExGaugeHUD is called
	// 3) After updateBattleOfflineVerHook ends, TickActorComponentsHook gets called many times in one frame
	// 4) This hook ends
	// 5) TickActorComponentsHook gets called some more times - it is known that Jam 22K/S/H aura effect gets ticked in one of these calls
	// 6) USkeletalMeshComponent::UpdateTransform is ran many times (we don't need this function, this is just for info)
	// 6.5) Meanwhile, in GUI thread, FUpdatePrimitiveTransformCommand::Apply gets called many times, and reads the data of the updated transforms (we don't need this info)
	// 7) The camera hook gets called in the game logic thread which prepares the camera data for this frame.
	// 8) REDAnywhereDispDraw hook gets called in EndScene.cpp - this happens even when pause menu is open or a match is not running (for ex. on main menu)

	// In rollback-affected online matches, the camera code runs multiple times per frame, from oldest frame to latest.
	// However, USkeletalMeshComponent::UpdateTransform is always called only once per frame.
	// It is also certain that in rollback-affected matches, camera code runs first, and only
	// then USkeletalMeshComponent::UpdateTransform/FUpdatePrimitiveTransformCommand::Apply happen.
	// 
	// When game pause menu is open in single player, USkeletalMeshComponent::UpdateTransform does not get called,
	// and this hook is also not called.
	if (!shutdown) {
		ignoreAllCalls = ignoreAllCallsButEarlier;
		endScene.onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(ignoreAllCalls);
	}
	orig_TickActors_FDeferredTickList_FGlobalActorIterator(param1, param2, param3, param4);
	if (!shutdown) {
		if (ignoreAllCalls) {
			TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty();
		}
	}
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty() {
	entityList.populate();
	if (drawJackoHouseHp) {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (!ent.isActive()) continue;
			drawJackoHouseHp((void*)ent.ent);
		}
	}
	if (getTrainingHud) {
		trainingHudTick(getTrainingHud());
		// the hook for trainingHudTick is in EndScene
	}
	if (drawExGaugeHUD) {
		char* battleHud = *(char**)(*aswEngine + REDHUD_BattleOffset);
		char field1 = *(char*)(battleHud + 0x45c);
		char* gameInfoBattle = *(char**)(*aswEngine + REDGameInfo_BattleOffset);
		char field2 = *(char*)(gameInfoBattle + 0x4c8);
		drawExGaugeHUD((void*)(*aswEngine + drawExGaugeHUDOffset), (field1 & 0x8) != 0 && (field2 & 0x2) != 0);
	}
	for (int i = 0; i < 2; ++i) {
		Entity pawn = entityList.slots[i];
		if (pawn.cmnActIndex() == CmnActJitabataLoop) {
			drawStunButtonMash(pawn);
		}
		drawStunLeverWithButtonMash(pawn);
	}
}

char Game::getPlayerSide() const {
	if (getGameMode() == GAME_MODE_NETWORK) {
		DWORD off = offsetof(RematchMenu, playerIndex);
		if (lastRematchMenuPlayerSide != -1) return lastRematchMenuPlayerSide;  // this fix is needed because when rematch menu is fading out but the next match hasn't started yet, 0x1734 is still 2
		if (isRematchMenuOpen()) return getRematchMenuPlayerSide();  // fix for rematch menu: 0x1734 holds 2 on it even if you're a direct participant of the match
		if (!playerSideNetworkHolder) return 2;
		// Big thanks to WorseThanYou for finding this value
		return *(char*)(*playerSideNetworkHolder + 0x1734);  // 0 for p1 side, 1 for p2 side, 2 for observer
	} else if (*gameDataPtr) {
		return *(char*)(*gameDataPtr + 0x44);  // this makes sense for training mode for example (maybe only all single player modes)
	} else {
		return 2;
	}
}

bool Game::bothPlayersHuman() const {
	if (!*aswEngine) return false;
	entityList.populate();
	return entityList.count >= 2
		&& !entityList.slots[0].isCpu()
		&& !entityList.slots[1].isCpu();
}

GameMode Game::getGameMode() const {
	if (!*gameDataPtr) return GAME_MODE_TRAINING;
	return (GameMode)*(*gameDataPtr + 0x45);
}

bool Game::isMatchRunning() const {
	if (!*aswEngine) return false;
	return *(unsigned int*)(*aswEngine + 4 + matchInfoOffset + 0x12C) != 0; // thanks to WorseThanYou for finding this
}

bool Game::isTrainingMode() const {
	return getGameMode() == GAME_MODE_TRAINING;
}

bool Game::isNonOnline() const {
	GameMode gameMode = getGameMode();
	return gameMode == GAME_MODE_ARCADE
		|| gameMode == GAME_MODE_CHALLENGE
		|| gameMode == GAME_MODE_REPLAY
		|| gameMode == GAME_MODE_STORY
		|| gameMode == GAME_MODE_TRAINING
		|| gameMode == GAME_MODE_TUTORIAL
		|| gameMode == GAME_MODE_VERSUS;
}

bool Game::currentModeIsOnline() const {
	return !isNonOnline();
}

int Game::getBurst(int team) const {
	if (!burstOffset || team != 0 && team != 1) return 0;
	return *(int*)(*aswEngine + burstOffset + team * 4);
}

void Game::destroyAswEngineHook() {
	if (!game.shutdown) {
		if (*aswEngine) {
			logwrap(fputs("Asw Engine destroyed\n", logfile));
			endScene.onAswEngineDestroyed();
			game.lastSavedPositionX[0] = -252000;
			game.lastSavedPositionX[1] = 252000;
		}
		for (int i = 0; i < 2; ++i) {
			game.prevScores[i] = 0;
			game.changedScore[i] = false;
		}
		game.lastRematchMenuPlayerSide = -1;
	}
	game.orig_destroyAswEngine();
}

void Game::UWorld_TickHook(void* thisArg, ELevelTick TickType, float DeltaSeconds) {
	
	if (!shutdown) {
		IsPausedCallCount = 0;
		
		ignoreAllCallsButEarlier = false;
		if (freezeGame && *aswEngine) {
			slowmoSkipCounter = 0;
			if (!allowNextFrame) {
				ignoreAllCallsButEarlier = true;
			}
			allowNextFrame = false;
		}
		if (slowmoGame && *aswEngine) {
			++slowmoSkipCounter;
			if ((int)slowmoSkipCounter < settings.slowmoTimes) {
				ignoreAllCallsButEarlier = true;
			} else {
				slowmoSkipCounter = 0;
			}
		} else {
			slowmoSkipCounter = 0;
		}
		
		bool unused;
		bool normalMode = altModes.isGameInNormalMode(&unused);
		if (*aswEngine && ignoreAllCallsButEarlier && normalMode) {
			for (int i = 0; i < 4; ++i) {
				onNeedRememberPress(i, playPressed + i, BUTTON_CODE_PLAY);
				onNeedRememberPress(i, recordPressed + i, BUTTON_CODE_RECORD);
				onNeedRememberPress(i, resetPressed + i, BUTTON_CODE_MENU_RESET);
				onNeedRememberPress(i, unknownPressed + i, BUTTON_CODE_MENU_UNKNOWN);
			}
		} else if (*aswEngine && normalMode) {
			for (int i = 0; i < 4; ++i) {
				onNeedForcePress(i, playPressed + i, BUTTON_CODE_PLAY);
				onNeedForcePress(i, recordPressed + i, BUTTON_CODE_RECORD);
				onNeedForcePress(i, resetPressed + i, BUTTON_CODE_MENU_RESET);
				onNeedForcePress(i, unknownPressed + i, BUTTON_CODE_MENU_UNKNOWN);
			}
		} else {
			for (int i = 0; i < 4; ++i) {
				playPressed[i] = false;
				recordPressed[i] = false;
				resetPressed[i] = false;
				unknownPressed[i] = false;
			}
		}
	}
	
	bool hadAsw = *aswEngine != nullptr;
	endScene.onUWorld_TickBegin();
	game.orig_UWorld_Tick(thisArg, TickType, DeltaSeconds);
	endScene.onUWorld_Tick();
	if (!shutdown && !game.orig_destroyAswEngine && hadAsw && *aswEngine == nullptr) {
		logwrap(fputs("Asw Engine destroyed\n", logfile));
		endScene.onAswEngineDestroyed();
	}
	if (!shutdown && (!*aswEngine || !ignoreAllCallsButEarlier)) {
		for (int i = 0; i < 4; ++i) {
			playPressed[i] = false;
			recordPressed[i] = false;
			resetPressed[i] = false;
			unknownPressed[i] = false;
		}
	}
	ignoreAllCallsButEarlier = false;
	ignoreAllCalls = false;
}

void Game::HookHelp::UWorld_TickHook(ELevelTick TickType, float DeltaSeconds) {
	game.UWorld_TickHook(this, TickType, DeltaSeconds);
}

void Game::HookHelp::drawExGaugeHUDHook(int param_1) {
	if (gifMode.modDisabled || !(gifMode.gifModeOn || gifMode.gifModeToggleHudOnly)) {
		game.orig_drawExGaugeHUD((void*)this, param_1);
	}
}

void Game::drawStunLeverWithButtonMash(Entity pawn) {
	if (!drawStunMashPtr) return;
	if (pawn.hp() * 10000 / 420 <= 0) return;
	int dizzyMashAmountLeft = pawn.dizzyMashAmountLeft();
	if (dizzyMashAmountLeft <= 0) return;
	float bar = (float)dizzyMashAmountLeft / (float)pawn.dizzyMashAmountMax();
	drawStunMashPtr((void*)pawn, bar, true, true);
}

void Game::drawStunButtonMash(Entity pawn) {
	if (!drawStunMashPtr) return;
	if (pawn.hp() * 10000 / 420 <= 0) return;
	if ((*(DWORD*)(pawn + 0x710 + 0x14) & 0x8000000) != 0) return;
	if (pawn.dizzyMashAmountLeft() > 0) return;
	int field = *(int*)(pawn + 0x710 + 0x168);
	int amount = pawn.currentAnimDuration() - *(int*)(pawn + 0x24e04) / 10 - 1 + field;
	float bar = (float)amount;
	if (amount < 0) {
		bar = float_max;
	}
	drawStunMashPtr((void*)pawn, 1.F - bar / (float)field, true, false);
}

bool Game::HookHelp::UWorld_IsPausedHook() {
	// This function is called from UWorld::Tick several times.
	if (!game.shutdown) {
		// If UWorld->DemoRecDriver is not nullptr, we might get an extra call at the start that messes us up.
		// It's a: 'Fake NetDriver for capturing network traffic to record demos'.
		// Probably will never be non-0.
		++game.IsPausedCallCount;
		if (game.IsPausedCallCount == 2) {
			if (game.ignoreAllCallsButEarlier) {
				return true;
			}
		}
	}
	return game.orig_UWorld_IsPaused((void*)this);
}

bool Game::buttonPressed(int padInd, bool isMenu, DWORD code) {
	if (!inputsHolder) return false;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	DWORD inputs = *(DWORD*)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x10);
	return (inputs & code) != 0;
}

DWORD Game::getPressedButtons(int padInd, bool isMenu) {
	if (!inputsHolder) return false;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	return *(DWORD*)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x10);
}

bool Game::buttonHeld(int padInd, bool isMenu, DWORD code) {
	if (!inputsHolder) return false;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	BYTE* step3 = *(BYTE**)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x8);
	DWORD inputs = *(DWORD*)(step3 + 0xc);
	return (inputs & code) != 0;
}

DWORD Game::getHeldButtons(int padInd, bool isMenu) {
	if (!inputsHolder) return false;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	BYTE* step3 = *(BYTE**)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x8);
	return *(DWORD*)(step3 + 0xc);
}

void Game::setButtonPressed(int padInd, bool isMenu, DWORD code) {
	if (!inputsHolder) return;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	DWORD* inputs = (DWORD*)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x10);
	*inputs |= code;
}

void Game::onNeedRememberPress(int padInd, bool* pressed, ButtonCode code) {
	onNeedRememberPress(padInd, pressed, false, code);
}

void Game::onNeedRememberPress(int padInd, bool* pressed, ButtonCodeMenu code) {
	onNeedRememberPress(padInd, pressed, true, code);
}

void Game::onNeedRememberPress(int padInd, bool* pressed, bool isMenu, DWORD code) {
	bool isPressed = buttonPressed(padInd, isMenu, code);
	if (isPressed) {
		*pressed = true;
	} else if (!buttonHeld(padInd, isMenu, code)) {
		*pressed = false;
	}
}

void Game::onNeedForcePress(int padInd, bool* pressed, ButtonCode code) {
	onNeedForcePress(padInd, pressed, false, code);
}

void Game::onNeedForcePress(int padInd, bool* pressed, ButtonCodeMenu code) {
	onNeedForcePress(padInd, pressed, true, code);
}

void Game::onNeedForcePress(int padInd, bool* pressed, bool isMenu, DWORD code) {
	if (*pressed) {
		if (buttonHeld(padInd, isMenu, code)) {
			setButtonPressed(padInd, isMenu, code);
		}
		*pressed = false;
	}
}

bool Game::isFading() const {
	if (!*aswEngine) return false;
	BYTE* REDHUD_Battle = *(BYTE**)(*aswEngine + REDHUD_BattleOffset);
	BYTE* vtable = *(BYTE**)REDHUD_Battle;
	getGameViewportClient_t getGameViewportFunc = *(getGameViewportClient_t*)(vtable + 0x384);  // if this offset breaks, search for string AREDHUDexecGetGameViewportClient
	// it will be used in only one place, and at offset +0x4 from that place is the corresponding function pointer.
	// go into this function. In it, it's doing uVar1 = (**(code **)(*this + 900))(); 900 is the up-to-date offset in the vtable to this function. Change it here if it's different
	BYTE* REDGameViewportClient = (BYTE*)getGameViewportFunc(REDHUD_Battle);
	return (*(DWORD*)(REDGameViewportClient + 0x128) & 0x1) != 0;  // 0x128 & 0x1 is FadeDraw, & 0x2 is FadeEnd and was spotted to be set long after fade is over
	// if this offset breaks, you need to list the members of the REDGameViewportClient UObject and find it by 'FadeDraw' name.
	// Listing contents of UObjects is easy. First you need to find the FNameNames. Search for this byte sequence in RAM:
	// 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 4E 6F 6E 65 00 00 00 00 00 00 00 00 00 02 00 00 00 00 00 00 00 42 79 74 65 50 72 6F 70 65 72 74 79
	// It will be at an ADDRESS. Next search for Array of Byte again, but the array is made up of this ADDRESS, then ADDRESS + 0x15, and then ADDRESS + 0x32.
	// For example, if ADDRESS is 0x3c40000, then search for 00 00 C4 03 15 00 C4 03 32 00 C4 03.
	// This byte sequence will be at an ADDRESS (forget the old ADDRESS). This ADDRESS is the current value of FNameNames pointer.
	// To find where FNamesNames is relative to the in-memory EXE file,
	// just search for '4 Bytes' and enter the ADDRESS. The first search result will be 'GuiltyGearXrd.exe+something' and that's where it is.
	// Now that you've found FNameNames, you need to know a bit about UObject and UClass structs.
	// At 0x2c, every UObject has an FName field called 'Name'. FName is just a number, so to get a string from it you have to (char*)(*(BYTE**)(*(BYTE**)FNameNames + (int)fName * 4) + 0x10)
	// At 0x34, every UObject has a pointer to its UClass. UClass extends UObject.
	// Because every UClass object is of type Class, to get the superclass you get it from another field in the UClass: 0x48.
	// There are two ways to list all members of a UClass: through Children (0x4c, UField*, extends UObject) or through PropertyLink (0x70, UProperty*, extends UField).
	// Each UField at 0x3c points to the next UField.
	// In each UField, the class (0x34) will be its type, and the name (0x2c) will be its name.
	// Each UProperty points to where it is inside the UObject by specifying an offset relative to the UObject's start. The offset is specified at 0x60 in UProperty. UFields don't have this.
	// Bool properties (class BoolProperty) also specify a bitmask at 0x70.
	// StructProperty specifies a nested type at 0x70, it's a ScriptStruct*.
	// MapProperty specifies key UProperty* at 0x70 and value UProperty* at 0x74 but those seem to be broken.
	// ArrayProperty specifies the type of its members at 0x70, a UProperty*.
	// ObjectProperty and ComponentProperty specify UClass* at 0x70.
	// ByteProperty specifies a UEnum* at 0x70. UEnum extends UField and has a TArray<FName> at 0x40.
	// TArray<TypeName> is a struct TArray<TypeName> {
	//   int ArrayNum;
	//   int ArrayMax;
	//   TypeName* Data;
	// };  FName is 8 bytes long but the index we need is at offset 0 and only 4 bytes long. The second 4 bytes are the number part of an FName and if it's non-0,
	//     an underscore is added to the string part of the name, followed by a decimal print of the number-1. For example, FName 0x00000000 0x00000000 is "None", but
	//     FName 0x00000000 0x00000001 is "None_0"
	// To list fields of a ScriptStruct you have to treat it as a UClass and list its members directly either through Children or PropertyLink.
	// I have a tool that automates some of this, but it's shit just make your own
}

bool Game::isRoundend() const {
	if (!roundendSuperfreezeCounterOffset) return false;
	if (!*aswEngine) return false;
	return *(int*)(*aswEngine + 4 + roundendSuperfreezeCounterOffset) > 0;
}

DummyRecordingMode Game::getDummyRecordingMode() const {
	if (!getTrainingHud) return DUMMY_MODE_IDLE;
	char* trainingStruct = getTrainingHud();
	return *(DummyRecordingMode*)(trainingStruct);
}

BOOL& Game::postEffectOn() {
	static BOOL placeholder = 0;
	if (!postEffectOnPtr) return placeholder;
	return *postEffectOnPtr;
}

float Game::HookHelp::getRiscForUI_BackgroundHook() {
	if (settings.showComboProrationInRiscGauge) {
		Entity player = *(Entity*)((char*)this + 0x484);
		return (float)(player.riscResidual() / 2 + 6400) * 0.000078124998F;
	} else {
		return game.orig_getRiscForUI_Background((char*)this);
	}
}

float Game::HookHelp::getRiscForUI_ForegroundHook() {
	if (settings.showComboProrationInRiscGauge) {
		Entity player = *(Entity*)((char*)this + 0x484);
		return (float)(player.risc() / 2 + 6400) * 0.000078124998F;
	} else {
		return game.orig_getRiscForUI_Foreground((char*)this);
	}
}

int Game::getDangerTime() const {
	if (!dangerTimeOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + 4 + dangerTimeOffset);
}

bool Game::isStylish(Entity pawn) const {
	if (!isStylishPtr) return false;
	return isStylishPtr((void*)pawn.ent) != 0;
}

int Game::getStylishDefenseInverseModifier() const {
	if (!stylishDefenseInverseModifierOffset || !*aswEngine) return 100;
	return *(int*)(*aswEngine + stylishDefenseInverseModifierOffset);
}

int Game::getStylishBurstGainModifier() const {
	if (!stylishBurstGainModifierOffset || !*aswEngine) return 100;
	return *(int*)(*aswEngine + stylishBurstGainModifierOffset);
}

int Game::getHandicap(int playerIndex) const {
	if (!handicapsOffset || !*aswEngine) return 2;
	return *(int*)(*aswEngine + handicapsOffset + playerIndex * 4);
}

int Game::getTrainingSetting(TrainingSettingId setting) const {
	if (!getTrainingSettingPtr || !getTrainingHud) return 0;
	return getTrainingSettingPtr(getTrainingHud(), setting, 0);
}

InputRingBuffer* Game::getInputRingBuffers() const {
	if (!*aswEngine) return nullptr;
	return (InputRingBuffer*)(*aswEngine + 4 + inputRingBuffersOffset);
}

int Game::getMatchTimer() const {
	if (!*aswEngine) return 0;
	return *(int*)(*aswEngine + 4 + matchInfoOffset + 0xc);
}

BYTE* Game::getStaticFont() const {
	return *(BYTE**)(*gameDataPtr + 0x68);
}

void Game::setPositionResetTypeHookStatic() {
	game.setPositionResetTypeHook();
}

void Game::setPositionResetTypeHook() {
	orig_setPositionResetType();
	if (!isTrainingMode() || !settings.usePositionResetMod || gifMode.modDisabled) return;
	int padID = getPlayerPadID();
	DWORD heldBtns = getHeldButtons(padID, true);
	entityList.populate();
	int playerSide = getPlayerSide();
	bool needCorner = false;
	bool needLeftCorner = false;
	bool swapPlayers = false;
	if ((heldBtns & BUTTON_CODE_MENU_RIGHT) != 0) {
		if ((heldBtns & BUTTON_CODE_MENU_UP) != 0) {
			for (int i = 0; i < 2 && i < entityList.count; ++i) {
				Entity p = entityList.slots[i];
				if (p) {
					lastSavedPositionX[i] = p.x();
				}
			}
		} else {
			needCorner = true;
			swapPlayers = (heldBtns & BUTTON_CODE_MENU_DOWN) != 0;
		}
	} else if ((heldBtns & BUTTON_CODE_MENU_LEFT) != 0) {
		if ((heldBtns & BUTTON_CODE_MENU_UP) != 0) {
			// do nothing
		} else {
			needCorner = true;
			needLeftCorner = true;
			swapPlayers = (heldBtns & BUTTON_CODE_MENU_DOWN) != 0;
		}
	} else if ((heldBtns & BUTTON_CODE_MENU_UP) != 0) {
		lastSavedPositionX[1] = -252000;
		lastSavedPositionX[0] = 252000;
	} else if ((heldBtns & BUTTON_CODE_MENU_DOWN) != 0) {
		lastSavedPositionX[0] = -252000;
		lastSavedPositionX[1] = 252000;
	} else {
		// do nothing
	}
	
	if (needCorner) {
		if (swapPlayers) playerSide = 1 - playerSide;
		int cornerClose = 1515000 - settings.positionResetDistFromCorner;
		int cornerFar = cornerClose - settings.positionResetDistBetweenPlayers;
		lastSavedPositionX[1 - playerSide] = cornerClose * (needLeftCorner ? -1 : 1);
		lastSavedPositionX[playerSide] = cornerFar * (needLeftCorner ? -1 : 1);
	}
	
	numberOfPlayersReset = 0;
	
}

void Game::HookHelp::roundInitHook() {
	return game.roundInitHook(Entity{(char*)this});
}

void Game::roundInitHook(Entity pawn) {
	orig_roundInit((void*)pawn.ent);
	if (isTrainingMode() && settings.usePositionResetMod && !gifMode.modDisabled) {
		int team = pawn.team();
		int thisX = lastSavedPositionX[team];
		pawn.x() = thisX;
		int otherX = lastSavedPositionX[1 - team];
		bool facing = otherX < thisX;
		pawn.inputsFacingLeft() = facing;
		pawn.isFacingLeft() = facing;
		if (numberOfPlayersReset == 0) {
			*(int*)(*aswEngine + cameraCenterXOffset) = (lastSavedPositionX[0] + lastSavedPositionX[1]) / 2000;
		}
		++numberOfPlayersReset;
	}
}

void Game::clearInputHistory() {
	char* trainingStruct = getTrainingHud();
	memset(trainingStruct + 0x1c, 0, 4 * 2);
	memset(trainingStruct + 0x24, 0, 4 * 2);
	memset(trainingStruct + 0x2c, 0, 200 * 2);
	memset(trainingStruct + 0x1bc, 0, 200 * 2);
	memset(trainingStruct + 0x34c, false, 200);
	memset(trainingStruct + 0x414, 0, 4 * 2);
}

bool Game::is0xa8PreparingCamera() const {
	char* aswEngVal = *aswEngine;
	if (!aswEngVal || !aswEng0x1c710cOffset) return false;
	char* aswEng0x11c710c_ac = *(char**)(aswEngVal + 4 + aswEng0x1c710cOffset + 0xac);
	if (!aswEng0x11c710c_ac) return false;
	if (!aswEng0x11c710c_ac) return false;
	uintptr_t vtable = *(uintptr_t*)aswEng0x11c710c_ac;
	return vtable == normal0xa8ElementVtable
		&& *(int*)(aswEng0x11c710c_ac + 0x24) != 0;
}

const char* Game::formatGameMode(GameMode gameMode) {
	switch (gameMode) {
		case GameMode::GAME_MODE_ADVERTISE: return "Advertise";
		case GameMode::GAME_MODE_ARCADE: return "Arcade";
		case GameMode::GAME_MODE_CHALLENGE: return "Challenge";
		case GameMode::GAME_MODE_DEBUG_BATTLE: return "DebugBattle";
		case GameMode::GAME_MODE_DEGITALFIGURE: return "DigitalFigure";
		case GameMode::GAME_MODE_EVENT: return "Event";
		case GameMode::GAME_MODE_FISHING: return "Fishing";
		case GameMode::GAME_MODE_GALLERY: return "Gallery";
		case GameMode::GAME_MODE_INVALID: return "Invalid";
		case GameMode::GAME_MODE_KENTEI: return "Kentei";
		case GameMode::GAME_MODE_MAINMENU: return "MainMenu";
		case GameMode::GAME_MODE_MOM: return "MOM";
		case GameMode::GAME_MODE_NETWORK: return "Network";
		case GameMode::GAME_MODE_RANNYU_VERSUS: return "RannyuVersus";
		case GameMode::GAME_MODE_REPLAY: return "Replay";
		case GameMode::GAME_MODE_SPARRING: return "Sparring";
		case GameMode::GAME_MODE_STORY: return "Story";
		case GameMode::GAME_MODE_TRAINING: return "Training";
		case GameMode::GAME_MODE_TUTORIAL: return "Tutorial";
		case GameMode::GAME_MODE_UNDECIDED: return "Undecided";
		case GameMode::GAME_MODE_VERSUS: return "Versus";
		default: return "???";
	}
}

int Game::getRematchMenuPlayerSide() const {
	if (!allMenus) return 2;
	RematchMenu* rematchMenu = (RematchMenu*)allMenus[0x23];  // the P1 rematch menu
	// there is a second rematch menu at 0x24 (for P2), but, regarding the data we're interested in, they both hold the same values
	if (!rematchMenu->isNetwork || !rematchMenu->isDirectParticipant) return 2;
	return rematchMenu->playerIndex;
}

bool Game::isRematchMenuOpen() const {
	if (!currentMenu) return false;
	return *currentMenu == 0x22;
}

// Runs on the main thread. Called from _drawWinsHookAsm, declared in asmhooks.asm
// Must return TRUE if wins need to be drawn, FALSE otherwise
BOOL __cdecl drawWinsHook(RematchMenu* rematchMenu) {
	return game.drawWinsHook((BYTE*)rematchMenu);
}

BOOL Game::drawWinsHook(BYTE* rematchMenuByte) {
	#define DRAW_THE_WINS TRUE
	#define DONT_DRAW_WINS FALSE
	
	RematchMenu* rematchMenu = (RematchMenu*)rematchMenuByte;
	
	if (!isGameModeNetwork()) return DONT_DRAW_WINS;
	
	if (gifMode.modDisabled) return DRAW_THE_WINS;
	
	bool clearedChange = false;
	for (int i = 0x23; i <= 0x24; ++i) {
		RematchMenu* rematchMenuIter = (RematchMenu*)allMenus[i];
		if (!rematchMenuIter->isNetwork) return DRAW_THE_WINS;
		
		int index = rematchMenuIter->rematchMenuIndex;
		if (prevScores[index] != rematchMenuIter->score) {
			prevScores[index] = rematchMenuIter->score;
			if (!clearedChange) {
				clearedChange = true;
				changedScore[1 - index] = false;
			}
			changedScore[index] = true;
		}
	}
	
	if (!rematchMenu->isDirectParticipant) lastRematchMenuPlayerSide = 2;
	else lastRematchMenuPlayerSide = rematchMenu->playerIndex;
	
	if (settings.hideWins || settings.hideWinsDirectParticipantOnly && rematchMenu->isDirectParticipant) {
		if (rematchMenu->score == rematchMenu->maxScore  // players won't be able to rematch if this is the case
				|| settings.hideWinsExceptOnWins > 0
				&& rematchMenu->score == settings.hideWinsExceptOnWins
				&& changedScore[rematchMenu->rematchMenuIndex]) {
			return DRAW_THE_WINS;
		} else {
			return DONT_DRAW_WINS;
		}
	} else {
		return DRAW_THE_WINS;
	}
	
	#undef DRAW_THE_WINS
	#undef DONT_DRAW_WINS
}
