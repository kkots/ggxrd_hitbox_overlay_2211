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
#include "Graphics.h"
#include "UI.h"

char** aswEngine = nullptr;
appRealloc_t appRealloc = nullptr;

Game game;

extern "C" void drawWinsHookAsm();  // defined in asmhooks.asm
extern "C" BOOL __cdecl drawWinsHook(RematchMenu* rematchMenu);  // defined here

bool Game::onDllMain() {
	bool error = false;

	aswEngine = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"85 C0 78 74 83 F8 01",
		{-4, 0},
		&error, "aswEngine");
	
	// gameDataPtr is of type REDGameCommon** when not dereferenced
	gameDataPtr = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"33 C0 38 41 44 0F 95 C0 C3 CC",
		{-4, 0},
		&error, "gameDataPtr");

	netplayStruct = (char**)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 3c 10 75 10 a1 ?? ?? ?? ?? 85 c0 74 07 c6 80 0c 12 00 00 01 c3",
		{16, 0},
		NULL, "netplayStruct");

	uintptr_t UWorld_TickCallPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
		
		auto UWorld_TickHookPtr = &HookHelp::UWorld_TickHook;
		detouring.attach(&(PVOID&)(orig_UWorld_Tick),
			(PVOID&)UWorld_TickHookPtr,
			"UWorld_Tick");
	}
	
	if (orig_UWorld_IsPaused) {
		auto UWorld_IsPausedHookPtr = &HookHelp::UWorld_IsPausedHook;
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
		std::vector<char> burstMaskForCaching;
		// ghidra sig: 8b 56 40 a1 ?? ?? ?? ?? 8b 84 90 ?? ?? ?? ?? 5f 5e 5d 5b 8b 4c 24 18 33 cc
		byteSpecificationToSigMask("8b 56 40 a1 rel(?? ?? ?? ??) 8b 84 90 ?? ?? ?? ?? 5f 5e 5d 5b 8b 4c 24 18 33 cc",
			burstSig, burstMask, nullptr, 0, &burstMaskForCaching);
		substituteWildcard(burstSig, burstMask, 0, aswEngine);
		burstOffset = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			burstSig,
			burstMask,
			{ 11, 0 },
			NULL, "burstOffset", burstMaskForCaching.data());
		
		std::vector<char> destroySig;
		std::vector<char> destroyMask;
		std::vector<char> destroyMaskForCaching;
		// ghidra sig: c7 05 ?? ?? ?? ?? 00 00 00 00
		byteSpecificationToSigMask("c7 05 rel(?? ?? ?? ??) 00 00 00 00",
			destroySig, destroyMask, nullptr, 0, &destroyMaskForCaching);
		substituteWildcard(destroySig, destroyMask, 0, aswEngine);
		orig_destroyAswEngine = (destroyAswEngine_t)sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			destroySig,
			destroyMask,
			{ -0x5A },
			NULL, "destroyAswEngine", destroyMaskForCaching.data());
	
		// offset from aswEngine to its field containing a pointer to an instance of AREDCamera_Battle class
		std::vector<char> cameraOffsetSig;
		std::vector<char> cameraOffsetSigMask;
		std::vector<char> cameraOffsetSigMaskForCaching;
		// ghidra sig: 8b 4c 24 18 83 c4 08 0b 4c 24 14 74 1e 8b 15 ?? ?? ?? ?? 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 75 09 55 e8 ?? ?? ?? ?? 83 c4 04
		byteSpecificationToSigMask("8b 4c 24 18 83 c4 08 0b 4c 24 14 74 1e 8b 15 rel(?? ?? ?? ??) 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 75 09 55 e8 ?? ?? ?? ?? 83 c4 04",
			cameraOffsetSig, cameraOffsetSigMask, nullptr, 0, &cameraOffsetSigMaskForCaching);
	
		substituteWildcard(cameraOffsetSig, cameraOffsetSigMask, 0, aswEngine);
		// pointer to REDCamera_Battle
		cameraOffset = (unsigned int)sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			cameraOffsetSig,
			cameraOffsetSigMask,
			{ 0x15, 0 },
			&error, "cameraOffset", cameraOffsetSigMaskForCaching.data());
		
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
		GUILTY_GEAR_XRD_EXE,
		"8d bd a4 04 00 00 bd 02 00 00 00 90 8b 0f 3b cb 74 05 e8 ?? ?? ?? ?? 83 c7 04 4d 75 ef ff 86",
		{ 31, 0 },
		&error, "aswEngineTickCountOffset");
	if (aswEngineTickCountOffset) {
		dangerTimeOffset = aswEngineTickCountOffset - 0x2dc;
		matchInfoOffset = aswEngineTickCountOffset + 0x284;
	}
	

	std::vector<char> sig;
	std::vector<char> mask;
	std::vector<char> maskForCaching;
	if (REDHUD_BattleOffset) {
		// ghidra sig: 8b 81 ?? ?? ?? ?? f6 80 5c 04 00 00 08 74 14 8b 54 24 14 f6 82 c8 04 00 00 02
		byteSpecificationToSigMask("8b 81 rel(?? ?? ?? ??) f6 80 5c 04 00 00 08 74 14 8b 54 24 14 f6 82 c8 04 00 00 02",
			sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)REDHUD_BattleOffset);
		uintptr_t drawExGaugeHUDCallPlace = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			nullptr, "drawExGaugeHUDCallPlace", maskForCaching.data());
		if (drawExGaugeHUDCallPlace) {
			drawExGaugeHUDOffset = *(DWORD*)(drawExGaugeHUDCallPlace + 40);
			drawExGaugeHUD = (drawExGaugeHUD_t)followRelativeCall(drawExGaugeHUDCallPlace + 44);
			orig_drawExGaugeHUD = drawExGaugeHUD;
			
			auto drawExGaugeHUDHookPtr = &HookHelp::drawExGaugeHUDHook;
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
		nullptr, "_JitabataRecover", nullptr);
	
	uintptr_t stunmashDrawingPlace = 0;
	
	if (_JitabataRecover) {
		// ghidra sig: 53 53 53 68 ?? ?? ?? ?? 6a 15
		byteSpecificationToSigMask("53 53 53 68 rel(?? ?? ?? ??) 6a 15",
			sig, mask, nullptr, 0, &maskForCaching);
		substituteWildcard(sig, mask, 0, (void*)_JitabataRecover);
		
		stunmashDrawingPlace = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			sig,
			mask,
			{ 0x2b2 },
			nullptr, "stunmashDrawingPlace", maskForCaching.data());
		
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
		GUILTY_GEAR_XRD_EXE,
		"e8 ?? ?? ?? ?? bb 00 00 01 00 bf 00 00 80 00",
		{ -12, 0 },
		nullptr, "inputsHolderCallPlace");
	
	drawJackoHouseHp = (drawJackoHouseHp_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"74 19 8b 86 40 25 00 00 39 86 3c 25 00 00 7d 0b",
		{ -0x26 },
		nullptr, "drawJackoHouseHp");
	
	roundendSuperfreezeCounterOffset = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"83 bf ?? ?? ?? ?? 00 7e 31 33 d2 8b c2 3b 97 b0 00 00 00 7d 25 8b 84 87 f8 01 00 00 42 85 c0 74 19 8b 88 18 01 00 00 f6 c1 01 75 df 81 c9 00 00 00 04 89 88 18 01 00 00 eb d1",
		{ 2, 0 },
		nullptr, "roundendSuperfreezeCounter");
	if (roundendSuperfreezeCounterOffset) {
		aswEng0x1c710cOffset = roundendSuperfreezeCounterOffset + 0x35b4;  // probably better to make a dedicated sigscan, but we have too many already and it's slowing down the injection
	}
	
	postEffectOnPtr = (BOOL*)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"89 8e a4 01 00 00 8b 15 ?? ?? ?? ?? 89 96 a8 01 00 00 39 1d ?? ?? ?? ?? 0f 94 c0 33 c9 89 86 ac 01 00 00",
		{ 20, 0 },
		nullptr, "postEffectOn");
	
	orig_getRiscForUI_Background = (getRiscForUI_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"66 0f 6e 80 34 4e 02 00",
		{ -15 },
		nullptr, "getRiscForUIBackground");
	if (orig_getRiscForUI_Background) {
		auto getRiscForUI_BackgroundHookPtr = &HookHelp::getRiscForUI_BackgroundHook;
		detouring.attach(&(PVOID&)(orig_getRiscForUI_Background),
			(PVOID&)getRiscForUI_BackgroundHookPtr,
			"getRiscForUIBackground");
		
		orig_getRiscForUI_Foreground = (getRiscForUI_t)((char*)orig_getRiscForUI_Background - 0x40);
	}
	
	if (orig_getRiscForUI_Foreground) {
		auto getRiscForUI_ForegroundHookPtr = &HookHelp::getRiscForUI_ForegroundHook;
		detouring.attach(&(PVOID&)(orig_getRiscForUI_Foreground),
			(PVOID&)getRiscForUI_ForegroundHookPtr,
			"getRiscForUIForeground");
	}
	
	// Thanks to WorseThanYou for finding the location of the input ring buffers, their sizes and their structure
	inputRingBuffersOffset = (unsigned int)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 44 24 14 8b d7 6b d2 7e 56 8d 8c 02 ?? ?? ?? ?? e8 ?? ?? ?? ?? 66 89 74 7c 18",
		{ 13, 0 },
		nullptr, "inputRingBuffersOffset");
	
	if (settings.usePositionResetMod) {
		if (!sigscanAndHookPositionResetAndGetPlayerPadID()) return false;
	}
	
	uintptr_t placeInFunction0x10_OfNormalElementFor_AswSubEng_0x1c710c_0xac = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
			(const char*)&startOfFunction0x10, 4,
			"func0x10InRData", "rel_GuiltyGearXrd.exe(????)");
	}
	if (func0x10InRData) {
		normal0xa8ElementVtable = func0x10InRData - 0x10;
	}
	
	uintptr_t menuUsage = sigscanOffset(GUILTY_GEAR_XRD_EXE,
		// use only a part of function to find it faster
		"83 b8 54 01 00 00 00 75 03 4e 79 eb",
		{ -15 },
		nullptr,
		"MenuUsage");
	if (menuUsage) {
		// validate the whole function, I'm going to need bits and pieces from it
		byteSpecificationToSigMask("56 be 65 00 00 00 56 e8 ?? ?? ?? ?? 83 c4 04 83 b8 54 01 00 00 00 75 03 4e 79 eb 89 35 ?? ?? ?? ?? 5e c3", sig, mask);
		if (sigscan(menuUsage, menuUsage + 0x23, sig.data(), mask.data(), nullptr, nullptr) != menuUsage) {
			menuUsage = 0;
		}
	}
	if (menuUsage) {
		currentMenu = *(int**)(menuUsage + 29);
		
		uintptr_t getMenu = followRelativeCall(menuUsage + 7);
		byteSpecificationToSigMask("8b 44 24 04 8b 04 85 ?? ?? ?? ?? c3", sig, mask);
		if (sigscan(getMenu, getMenu + 12, sig.data(), mask.data(), nullptr, nullptr) == getMenu) {
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
		sizeof RF_MenuStr,
		"RF_MenuString", nullptr);
	
	uintptr_t RF_MenuUsage = 0;
	if (RF_MenuLoc) {
		char RF_DataUsageAr[5];
		RF_DataUsageAr[0] = 0x68;  // PUSH
		memcpy(RF_DataUsageAr + 1, &RF_MenuLoc, 4);
		RF_MenuUsage = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			RF_DataUsageAr,
			"xxxxx",
			nullptr,
			"RF_MenuUsage", "xrel_GuiltyGearXrd.exe(????)");
	}
	uintptr_t case0x21 = 0;
	if (RF_MenuUsage) {
		byteSpecificationToSigMask("ff 24 85 ?? ?? ?? ?? 81 4b 10 04 81 00 00", sig, mask);
		if (sigscan(RF_MenuUsage - 14, RF_MenuUsage, sig.data(), mask.data(), nullptr, nullptr) == RF_MenuUsage - 14) {
			case0x21 = *(DWORD*)(7 * 4 + *(DWORD*)(RF_MenuUsage - 14 + 3));
		}
	}
	uintptr_t isGameModeNetworkCall = 0;
	if (case0x21) {
		byteSpecificationToSigMask("e8 ?? ?? ?? ?? 85 c0 0f 84 ?? ?? ?? ??", sig, mask);
		// calls isGameModeNetwork, tests EAX and jumps if zero
		isGameModeNetworkCall = sigscan(case0x21, case0x21 + 0x120, sig.data(), mask.data(), nullptr, nullptr);
	}
	if (isGameModeNetworkCall) {
		isGameModeNetwork = (isGameModeNetwork_t)followRelativeCall(isGameModeNetworkCall);
		sig.resize(4);
		DWORD drawWinsHookAsmAddr = (DWORD)&drawWinsHookAsm;
		drawWinsHookAsmAddr = calculateRelativeCallOffset(isGameModeNetworkCall, drawWinsHookAsmAddr);
		memcpy(sig.data(), &drawWinsHookAsmAddr, 4);
		detouring.patchPlace(isGameModeNetworkCall + 1, sig);
	}
	
	if (settings.hideRankIcons) {
		hideRankIcons();
	}
	
	onConnectionTierChanged();
	
	return !error;
}

bool Game::sigscanAfterHitDetector() {
	uintptr_t getTrainingSettingCallPlace = 0;
	if (hitDetector.activeFrameHit) {
		getTrainingSettingCallPlace = sigscanForward(hitDetector.activeFrameHit,
			"6a 0b 89 6c 24 30 e8 ?? ?? ?? ?? 8b c8 >e8 ?? ?? ?? ??",
			0xc00);
	}
	
	if (getTrainingSettingCallPlace) {
		getTrainingSettingPtr = (getTrainingSetting_t)followRelativeCall(getTrainingSettingCallPlace);
		if (thisIsOurFunction((uintptr_t)getTrainingSettingPtr)) {
			getTrainingSettingPtr = endScene.orig_getTrainingSetting;  // you motherfucker, 
		}
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
		GUILTY_GEAR_XRD_EXE,
		"83 ec 18 53 55 57 8b 7c 24 28 8b 87 48 01 00 00 33 db 89 9f 44 01 00 00 3b c3 7d 22 8b 87 40 01 00 00 89 9f 48 01 00 00 3b c3 74 12 6a 08",
		&error, "TickActors_FDeferredTickList_FGlobalActorIterator");
	
	uintptr_t trainingHudCallPlace = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
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
	std::vector<char> updateBattleOfflineVerSigMaskForCaching;
	// ghidra sig: 89 7c 24 14 e8 ?? ?? ?? ?? 85 c0 74 0a 6a 01 e8 ?? ?? ?? ?? 83 c4 04 8b 0d ?? ?? ?? ?? 8b 81 ?? ?? ?? ?? 8b 80 7c 03 00 00
	byteSpecificationToSigMask("89 7c 24 14 e8 ?? ?? ?? ?? 85 c0 74 0a 6a 01 e8 ?? ?? ?? ?? 83 c4 04 8b 0d rel(?? ?? ?? ??) 8b 81 ?? ?? ?? ?? 8b 80 7c 03 00 00",
		updateBattleOfflineVerSig, updateBattleOfflineVerSigMask, nullptr, 0, &updateBattleOfflineVerSigMaskForCaching);
	substituteWildcard(updateBattleOfflineVerSig, updateBattleOfflineVerSigMask, 2, aswEngine);
	orig_updateBattleOfflineVer = (updateBattleOfflineVer_t)(
		(
			sigscanOffset(
				GUILTY_GEAR_XRD_EXE,
				updateBattleOfflineVerSig,
				updateBattleOfflineVerSigMask,
				&error, "updateBattleOfflineVer", updateBattleOfflineVerSigMaskForCaching.data()
			) - 0x30
		) & 0xFFFFFFF0
	);
	logwrap(fprintf(logfile, "Final location of updateBattleOfflineVer: %p\n", orig_updateBattleOfflineVer));

	orig_TickActorComponents = (TickActorComponents_t)sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"57 bf 01 00 00 00 39 7c 24 20 75 18 8b 06 8b 90 ec 01 00 00 8b ce ff d2 c7 44 24 10 00 00 00 00",
		{ -8 },
		&error, "TickActorComponents");

	return !error;
}

void Game::hookFrameByFraming() {
	
	detouring.attach(&(PVOID&)(orig_TickActors_FDeferredTickList_FGlobalActorIterator),
		Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookStatic,
		"TickActors_FDeferredTickList_FGlobalActorIterator");

	auto updateBattleOfflineVerHookPtr = &HookHelp::updateBattleOfflineVerHook;
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

// AActor* Actor,FLOAT DeltaSeconds (1/60),ELevelTick TickType (2), FDeferredTickList* DeferredList
void Game::TickActorComponentsHook(int param1, int param2, int param3, int param4) {
	if (!shutdown && !gifMode.modDisabled && !gifMode.mostModDisabled) {
		if (ignoreAllCalls) {
			bool found = false;
			auto itEnd = actorsToAllowTickFor.end();
			for (auto it = actorsToAllowTickFor.begin(); it != itEnd; ++it) {
				if (*it == (void*)param1) {
					actorsToAllowTickFor.erase(it);
					found = true;
					break;
				}
			}
			if (!found) return;
		} else {
			actorsToAllowTickFor.clear();
		}
	}
	orig_TickActorComponents(param1, param2, param3, param4);
}

void Game::HookHelp::updateBattleOfflineVerHook(int param1) {
	return game.updateBattleOfflineVerHook((char*)this, param1);
}

void Game::updateBattleOfflineVerHook(char* thisArg, int param1) {
	if (!shutdown && ignoreAllCalls && !gifMode.modDisabled && !gifMode.mostModDisabled) {
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
	// However, USkeletalMeshComponent::UpdateTransform (for Player 1, let's say) is always called only once per frame.
	// It is also certain that in rollback-affected matches, camera code runs first, and only
	// then USkeletalMeshComponent::UpdateTransform/FUpdatePrimitiveTransformCommand::Apply happen.
	// 
	// When game pause menu is open in single player, USkeletalMeshComponent::UpdateTransform does not get called,
	// and this hook is also not called.
	if (!shutdown) {
		ignoreAllCalls = ignoreAllCallsButEvenEarlier;
	}
	orig_TickActors_FDeferredTickList_FGlobalActorIterator(param1, param2, param3, param4);
	if (!shutdown && !gifMode.modDisabled && !gifMode.mostModDisabled) {
		if (ignoreAllCalls) {
			TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty();
		}
	}
	camera.lastGameTickRanFine = !ignoreAllCalls;
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
		if (!netplayStruct) return 2;
		// Big thanks to WorseThanYou for finding this value
		return *(char*)(*netplayStruct + 0x1734);  // 0 for p1 side, 1 for p2 side, 2 for observer
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
	
	gifMode.mostModDisabled = settings.disableMostOfTheModInOnline
		&& game.getGameMode() == GAME_MODE_NETWORK
		&& game.getPlayerSide() != 2
		&& aswEngine
		&& *aswEngine;
	
	if (!shutdown && !gifMode.modDisabled && !gifMode.mostModDisabled) {
		IsPausedCallCount = 0;
		
		ignoreAllCallsButEvenEarlier = false;
		if (freezeGame && *aswEngine) {
			if (!allowNextFrame) {
				ignoreAllCallsButEvenEarlier = true;
			}
			allowNextFrame = false;
		}
		
		bool unused;
		bool normalMode = altModes.isGameInNormalMode(&unused);
		if (*aswEngine && ignoreAllCallsButEvenEarlier && normalMode) {
			bool successfulSigscan = sigscanTrainingStructProcessPlayRecordReset() && getTrainingHud;
			if (successfulSigscan) {
				bool firePress = false;
				int padID = getPlayerPadID();
				bool pressedPlay = buttonPressed(padID, false, BUTTON_CODE_PLAY);
				bool pressedRecord = buttonPressed(padID, false, BUTTON_CODE_RECORD);
				bool pressedReset = buttonPressed(padID, false, BUTTON_CODE_MENU_RESET);
				bool holdPlay = buttonHeld(padID, false, BUTTON_CODE_PLAY);
				bool holdRecord = buttonHeld(padID, false, BUTTON_CODE_RECORD);
				bool holdReset = buttonHeld(padID, false, BUTTON_CODE_MENU_RESET);
				if (
					(
						pressedPlay && !pressedRecord
						|| pressedRecord && !pressedPlay
					)
					&& !pressedReset
					&& !holdReset
				) {
					playRecordFired = false;
					playOrRecordPressCounter = 0;
					if (pressedPlay) {
						isPlayRecord = PlayRecordEnum_Play;
					} else {  // pressedRecord
						isPlayRecord = PlayRecordEnum_Record;
					}
				} else if (
					playOrRecordPressCounter != INT_MAX
					&& (
						isPlayRecord == PlayRecordEnum_Play && holdPlay && !holdRecord && !pressedRecord
						|| isPlayRecord == PlayRecordEnum_Record && holdRecord && !holdPlay && !pressedPlay
					)
					&& !pressedReset
					&& !holdReset
				) {
					++playOrRecordPressCounter;
					if (playOrRecordPressCounter == 3 && !playRecordFired) {
						firePress = true;
						playRecordFired = true;
					}
				} else if (playOrRecordPressCounter != INT_MAX) {
					if (!playRecordFired) {
						if (!holdPlay
								&& !pressedPlay
								&& !holdRecord
								&& !pressedRecord
								&& !pressedReset
								&& !holdReset) {
							// a short press, shorter than 3f
							playRecordFired = true;
							firePress = true;
						} else if (isPlayRecord == PlayRecordEnum_Play) {
							playPressed[padID] = holdPlay;
						} else {  // isPlayRecord == PlayRecordEnum_Record
							recordPressed[padID] = holdRecord;
						}
					}
					playOrRecordPressCounter = INT_MAX;
				}
				if (firePress) {
					if (isPlayRecord == PlayRecordEnum_Play) {
						if (!pressedPlay) {
							setButtonPressed(padID, false, BUTTON_CODE_PLAY);
						}
					} else {  // isPlayRecord == PlayRecordEnum_Record
						if (!pressedRecord) {
							setButtonPressed(padID, false, BUTTON_CODE_RECORD);
						}
					}
					doNotIncrementSlotInputsIndex = true;
					trainingStructProcessPlayRecordReset(getTrainingHud());
					doNotIncrementSlotInputsIndex = false;
					if (isPlayRecord == PlayRecordEnum_Play) {
						if (!pressedPlay) {
							setButtonNotPressed(padID, false, BUTTON_CODE_PLAY);
						}
					} else {  // isPlayRecord == PlayRecordEnum_Record
						if (!pressedRecord) {
							setButtonNotPressed(padID, false, BUTTON_CODE_RECORD);
						}
					}
				}
			}
			for (int i = 0; i < 4; ++i) {
				// fallback to old method: will only register play/record being held when advancing to the next frame
				if (playOrRecordPressCounter == INT_MAX) {
					onNeedRememberPress(i, playPressed + i, BUTTON_CODE_PLAY);
					onNeedRememberPress(i, recordPressed + i, BUTTON_CODE_RECORD);
				}
				onNeedRememberPress(i, resetPressed + i, BUTTON_CODE_MENU_RESET);
				onNeedRememberPress(i, unknownPressed + i, BUTTON_CODE_MENU_UNKNOWN);
			}
		} else if (*aswEngine && normalMode) {
			for (int i = 0; i < 4; ++i) {
				if (onNeedForcePress(i, playPressed + i, BUTTON_CODE_PLAY)) playRecordFired = true;
				if (onNeedForcePress(i, recordPressed + i, BUTTON_CODE_RECORD)) playRecordFired = true;
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
	if (!shutdown && (!*aswEngine || !ignoreAllCallsButEvenEarlier) && !gifMode.modDisabled && !gifMode.mostModDisabled) {
		for (int i = 0; i < 4; ++i) {
			playPressed[i] = false;
			recordPressed[i] = false;
			resetPressed[i] = false;
			unknownPressed[i] = false;
		}
	}
	ignoreAllCallsButEvenEarlier = false;
	ignoreAllCalls = false;
}

void Game::HookHelp::UWorld_TickHook(ELevelTick TickType, float DeltaSeconds) {
	game.UWorld_TickHook(this, TickType, DeltaSeconds);
}

void Game::HookHelp::drawExGaugeHUDHook(int param_1) {
	if (gifMode.modDisabled || gifMode.mostModDisabled || !(gifMode.gifModeOn || gifMode.gifModeToggleHudOnly)) {
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
	if (!game.shutdown && !gifMode.modDisabled && !gifMode.mostModDisabled) {
		// If UWorld->DemoRecDriver is not nullptr, we might get an extra call at the start that messes us up.
		// It's a: 'Fake NetDriver for capturing network traffic to record demos'.
		// Probably will never be non-0.
		++game.IsPausedCallCount;
		if (game.IsPausedCallCount == 2) {
			if (game.ignoreAllCallsButEvenEarlier) {
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
	setButtonPressedNotPressed(padInd, isMenu, code, true);
}

void Game::setButtonNotPressed(int padInd, bool isMenu, DWORD code) {
	setButtonPressedNotPressed(padInd, isMenu, code, false);
}

void Game::setButtonPressedNotPressed(int padInd, bool isMenu, DWORD code, bool isPressed) {
	if (!inputsHolder) return;
	BYTE* step1 = *(BYTE**)inputsHolder;
	BYTE* step2 = *(BYTE**)(step1 + 0x28);
	DWORD* inputs = (DWORD*)(step2 + 0x38 + 0x38 * padInd + !isMenu * 0x1c + 0x10);
	if (!isPressed) {
		*inputs &= ~code;
	} else {
		*inputs |= code;
	}
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

bool Game::onNeedForcePress(int padInd, bool* pressed, ButtonCode code) {
	return onNeedForcePress(padInd, pressed, false, code);
}

bool Game::onNeedForcePress(int padInd, bool* pressed, ButtonCodeMenu code) {
	return onNeedForcePress(padInd, pressed, true, code);
}

bool Game::onNeedForcePress(int padInd, bool* pressed, bool isMenu, DWORD code) {
	bool returnValue = false;
	if (*pressed) {
		if (buttonHeld(padInd, isMenu, code)) {
			setButtonPressed(padInd, isMenu, code);
			returnValue = true;
		}
		*pressed = false;
	}
	return returnValue;
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

// this feature is supposed to work even if mod is disabled
float Game::HookHelp::getRiscForUI_BackgroundHook() {
	if (settings.showComboProrationInRiscGauge) {
		Entity player = *(Entity*)((char*)this + 0x484);
		return (float)(player.riscResidual() / 2 + 6400) * 0.000078124998F;
	} else {
		return game.orig_getRiscForUI_Background((char*)this);
	}
}

// this feature is supposed to work even if mod is disabled
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
		if (numberOfPlayersReset == 2) {
			numberOfPlayersReset = 0;
		}
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

void Game::patchDrawIcon(uintptr_t addr) {
	std::vector<char> newData(4);
	if (!drawIcon) {
		drawIcon = (drawIcon_t)followRelativeCall(addr);
	}
	DWORD hookAddr = (DWORD)&Game::hiddenRankDrawIcon;
	int hookOffset = calculateRelativeCallOffset(addr, hookAddr);
	memcpy(newData.data(), &hookOffset, 4);
	detouring.patchPlace(addr + 1, newData);
}

void Game::patchDrawTextureProbably(uintptr_t addr) {
	std::vector<char> newData(4);
	if (!drawTextureProbably) {
		drawTextureProbably = (drawTextureProbably_t)followRelativeCall(addr);
	}
	DWORD hookAddr = (DWORD)&Game::hiddenRankDrawTextureProbably;
	int hookOffset = calculateRelativeCallOffset(addr, hookAddr);
	memcpy(newData.data(), &hookOffset, 4);
	detouring.patchPlace(addr + 1, newData);
}

void Game::hideRankIcons() {
	
	if (!drawRankInLobbyOverPlayersHeads) {
		drawRankInLobbyOverPlayersHeads = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			"6a 00 56 6a 00 6a 00 6a 04 6a 01 51 89 46 1c 8b c4 8d 4c 24 24 89 08",
			{ 0x20 },
			nullptr,
			"drawRankInLobbyOverPlayersHeads");
		
		if (drawRankInLobbyOverPlayersHeads) patchDrawIcon(drawRankInLobbyOverPlayersHeads);
	}
	
	if (!drawRankInLobbySearchMemberList) {
		drawRankInLobbySearchMemberList = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			// I will even put ESP offsets into here
			"6a 00 8d 4c 24 24 51 6a 00 6a 00 6a 04 6a 01 89 44 24 54 51 8b c4 8d 94 24 a4 01 00 00 89 10",
			{ 0x28 },
			nullptr,
			"drawRankInLobbySearchMemberList");
		
		if (drawRankInLobbySearchMemberList) patchDrawIcon(drawRankInLobbySearchMemberList);
	}
	
	if (!drawRankInLobbyMemberList_NonCircle) {
		drawRankInLobbyMemberList_NonCircle = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			// I will even put ESP offsets into here
			"8d 94 24 0c 02 00 00 52 89 84 24 2c 02 00 00",
			{ 0xf },
			nullptr,
			"drawRankInLobbyMemberList_NonCircle");
		
		if (drawRankInLobbyMemberList_NonCircle) patchDrawTextureProbably(drawRankInLobbyMemberList_NonCircle);
	}
	
	if (!drawRankInLobbyMemberList_Circle) {
		drawRankInLobbyMemberList_Circle = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			// I will even put ESP offsets into here
			"8d 8c 24 10 02 00 00 51 89 84 24 30 02 00 00",
			{ 0xf },
			nullptr,
			"drawRankInLobbyMemberList_Circle");
		
		if (drawRankInLobbyMemberList_Circle) patchDrawTextureProbably(drawRankInLobbyMemberList_Circle);
	}
	if (!detouring.isInTransaction()) finishedSigscanning();
	
}

int Game::hiddenRankDrawIcon(int iconIndex, DrawTextWithIconsParams* params, BOOL dontCommit) {
	if (!settings.hideRankIcons) {
		return game.drawIcon(iconIndex, params, dontCommit);
	}
	return 0;
}

void Game::hiddenRankDrawTextureProbably(void* params) {
	if (!settings.hideRankIcons) {
		game.drawTextureProbably(params);
	}
}

void Game::updateOnlineDelay() {
	if (!netplayStruct || !*netplayStruct
		|| !settings.overrideOnlineInputDelay) return;
	
	bool isFullscreen = graphics.isFullscreen();
	
	OnlineDelaySettings newSettings {
		false,
		isFullscreen,
		isFullscreen
			? settings.onlineInputDelayFullscreen
			: settings.onlineInputDelayWindowed
	};
	
	if (newSettings == onlineDelayLastSet) return;
	
	onlineDelayLastSet = newSettings;
	
	*(int*)(*netplayStruct + 0x1cec) = newSettings.delayToSet;
	*(int*)(*netplayStruct + 0x11fc) = newSettings.delayToSet;
}

bool Game::sigscanAndHookPositionResetAndGetPlayerPadID() {
	if (!orig_setPositionResetType) {
		if (!attemptedToSigscanPositionReset) {
			attemptedToSigscanPositionReset = true;
			// If WTY's position reset patch is applied, this function won't be found
			orig_setPositionResetType = (setPositionResetType_t)sigscanOffset(
				GUILTY_GEAR_XRD_EXE,
				"6a 02 56 e8 ?? ?? ?? ?? 8b c8 e8 ?? ?? ?? ?? 85 c0 74 13 e8 ?? ?? ?? ?? f7 d8 1b c0 f7 d8 40 a3 ?? ?? ?? ?? 5e c3 8b 0d ?? ?? ?? ?? 6a 08 56 e8",
				{ -14 },
				nullptr, "setPositionResetType");
		}
	}
	if (orig_setPositionResetType && !attemptedToHookPositionReset) {
		
		bool wasInTransaction = detouring.isInTransaction();
		if (!wasInTransaction) {
			detouring.beginTransaction(false);
		}
		
		attemptedToHookPositionReset = true;
		detouring.attach(&(PVOID&)(orig_setPositionResetType),
			setPositionResetTypeHookStatic,
			"setPositionResetType");
		
		getPlayerPadIDPtr = (getPlayerPadID_t)followRelativeCall((uintptr_t)orig_setPositionResetType + 1);
		
		uintptr_t roundInitUsage = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			// c7 44 24 __ ________ is MOV [ESP+???], ???
			"c7 44 24 08 00 00 00 00 c7 44 24 0c 00 00 00 00 c7 44 24 10 d5 04 00 00 c7 44 24 14 b7 05 00 00 "
			"c7 44 24 18 d2 05 00 00 c7 44 24 1c 3b 06 00 00 c7 44 24 20 49 fa ff ff c7 44 24 24 2b fb ff ff "
			"c7 44 24 28 c5 f9 ff ff c7 44 24 2c 2e fa ff ff c7 44 24 30 98 ff ff ff c7 44 24 34 68 00 00 00 "
			"c7 44 24 38 e0 fc ff ff c7 44 24 3c 20 03 00 00 c7 44 24 40 ff ff ff ff c7 44 24 44 01 00 00 00",
			nullptr, "roundInitUsage");
		
		if (!roundInitUsage) return false;
		orig_roundInit = (roundInit_t)sigscanBackwards(roundInitUsage, "83 ec", 0x1f0);
		if (!orig_roundInit) return false;
		
		auto roundInitHookPtr = &HookHelp::roundInitHook;
		detouring.attach(&(PVOID&)(orig_roundInit),
			(PVOID&)roundInitHookPtr,
			"roundInit");
		
		if (!wasInTransaction) {
			detouring.endTransaction();
		}
	}
	if (!getPlayerPadIDPtr && !attemptedToSigscanPlayedPadID) {
		attemptedToSigscanPlayedPadID = true;
		// backup plan in case WTY's position reset patch is applied
		getPlayerPadIDPtr = (getPlayerPadID_t)sigscanOffset(GUILTY_GEAR_XRD_EXE,
			"33 c0 38 41 44 56 0f 95 c0 0f b6 d0 8b 74 91 3c 33 d2",
			{ -6 },
			nullptr, "getPlayerPadID");
	}
	if (!detouring.isInTransaction()) finishedSigscanning();
	return true;
}

void Game::onUsePositionResetChanged() {
	if (settings.usePositionResetMod) {
		sigscanAndHookPositionResetAndGetPlayerPadID();
	}
}

int Game::getPlayerPadID() {
	if (!getPlayerPadIDPtr) {
		if (!sigscanAndHookPositionResetAndGetPlayerPadID()) return 0;
	}
	if (getPlayerPadIDPtr) {
		return getPlayerPadIDPtr();
	} else {
		return 0;
	}
}

bool Game::sigscanTrainingStructProcessPlayRecordReset() {
	if (attemptedToSigscanTrainingStructProcessPlayRecordReset) {
		return trainingStructProcessPlayRecordReset != nullptr;
	}
	attemptedToSigscanTrainingStructProcessPlayRecordReset = true;
	
	uintptr_t middleOfFunction = sigscanOffset(GUILTY_GEAR_XRD_EXE,
		"8b 1e 83 fb 05 74 0a b9 ff ff ff 7f 89 4e 10 eb 05",
		nullptr, "MiddleOfTrainingStructProcessPlayRecordReset");
	if (!middleOfFunction) return false;
	
	trainingStructProcessPlayRecordReset_t trainingStructProcessPlayRecordReset_temp =
		(trainingStructProcessPlayRecordReset_t)sigscanBackwards16ByteAligned(
		middleOfFunction, "6a ff 68 ?? ?? ?? ?? 64 a1 00 00 00 00", 0xb0);
	
	if (!trainingStructProcessPlayRecordReset_temp) return false;
	
	uintptr_t ifModePlayback = sigscanForward((uintptr_t)trainingStructProcessPlayRecordReset_temp,
		"83 f8 03 75 ?? >e8 ?? ?? ?? ?? 85 c0 75 ??",
		0x600);
	
	if (!ifModePlayback) return false;
	
	uintptr_t ifModeRecord = sigscanForward((uintptr_t)trainingStructProcessPlayRecordReset_temp,
		"83 3d ?? ?? ?? ?? 00 0f 85 ?? ?? ?? ?? 8b 06 83 f8 02 75 ?? >e8 ?? ?? ?? ?? 85 c0 0f 85",
		0x600);
	if (!ifModeRecord) return false;
	
	std::vector<char> newBytes(4);
	functionInIsPlayInsideProcessPlayRecordReset = (functionInIsPlayInsideProcessPlayRecordReset_t)followRelativeCall(ifModePlayback);
	int callOffset = calculateRelativeCallOffset(ifModePlayback, (uintptr_t)functionInIsPlayInsideProcessPlayRecordResetHook);
	memcpy(newBytes.data(), &callOffset, 4);
	detouring.patchPlace(ifModePlayback + 1, newBytes);
	
	callOffset = calculateRelativeCallOffset(ifModeRecord, (uintptr_t)functionInIsPlayInsideProcessPlayRecordResetHook);
	memcpy(newBytes.data(), &callOffset, 4);
	detouring.patchPlace(ifModeRecord + 1, newBytes);
	
	trainingStructProcessPlayRecordReset = trainingStructProcessPlayRecordReset_temp;
	finishedSigscanning();
	return true;
}

void* Game::functionInIsPlayInsideProcessPlayRecordResetHook() {
	void* result = game.functionInIsPlayInsideProcessPlayRecordReset();
	if (result) return result;
	if (game.doNotIncrementSlotInputsIndex) return (void*)(uintptr_t)1;
	return nullptr;
}

void Game::onConnectionTierChanged() {
	
	static bool isFirstTimePatch = true;
	static bool isPatched = false;
	static int patchedTier = 0;
	static BYTE oldCode[6];
	static bool conductedSearch = false;
	static uintptr_t place = 0;
	
	if (!(
		isFirstTimePatch
			? settings.overrideYourConnectionTierForFilter
			: settings.overrideYourConnectionTierForFilter != isPatched
				|| (
					!settings.overrideYourConnectionTierForFilter
					|| !isPatched
					|| patchedTier != settings.connectionTierToPretendAs
				)
	)) return;
	
	struct OnExit {
		~OnExit() {
			isFirstTimePatch = false;
			isPatched = settings.overrideYourConnectionTierForFilter;
			patchedTier = settings.connectionTierToPretendAs;
		}
	} onExit;
	
	if (!conductedSearch) {
		conductedSearch = true;
		place = sigscanOffset(
			GUILTY_GEAR_XRD_EXE,
			"8b 94 81 98 bc 00 00 8d 8c 81 98 bc 00 00 8b 42 08 ff d0 8b 48 44 51 >e8",
			nullptr,
			"connectionTierPlaceForBypassingFilter");
		
		if (!detouring.isInTransaction()) finishedSigscanning();
	}
	if (!place) return;
	
	std::vector<char> workVector(sizeof oldCode);
	
	BYTE* functionStart = (BYTE*)followRelativeCall(place);
	if (!settings.overrideYourConnectionTierForFilter) {
		if (isFirstTimePatch) return;
		memcpy(workVector.data(), oldCode, sizeof oldCode);
		detouring.patchPlaceNoBackup((uintptr_t)functionStart, workVector);
		return;
	}
	
	workVector[0] = '\xB8';
	int theValue = settings.connectionTierToPretendAs;
	if (theValue < 0) theValue = 0;
	if (theValue > 4) theValue = 4;
	memcpy(workVector.data() + 1, &theValue, 4);
	workVector[5] = '\xC3';
	
	if (isFirstTimePatch) {
		memcpy(oldCode, functionStart, sizeof oldCode);
		// small chance of crash. We need to play around with EIP just like Microsoft Detours does. Actually we could hook the whole function instead of patching it
		detouring.patchPlace((uintptr_t)functionStart, workVector);
		return;
	}
	
	detouring.patchPlaceNoBackup((uintptr_t)functionStart, workVector);
}

int Game::currentPlayerControllingSide() const {
	DummyRecordingMode recordingMode = getDummyRecordingMode();
	
	int playerSide = getPlayerSide();
	if (playerSide != 0 && playerSide != 1) playerSide = 0;
	
	if (recordingMode == DUMMY_MODE_CONTROLLING
			|| recordingMode == DUMMY_MODE_RECORDING) {
		return 1 - playerSide;
	}
	
	return playerSide;
}

const void* Game::readFName(int fname, bool* isWide) {
	if (!fnameNamesPtr) {
		if (!sigscanFNamesAndAppRealloc() && !fnameNamesPtr) return nullptr;
	}
	// FNameNames may change location when loading new characters, as new names get added and it relocates its storage
	FNameEntry* entry = (*fnameNamesPtr)[fname];
	if (entry->Index & 1) {
		*isWide = true;
		return (const wchar_t*)entry->data;
	} else {
		*isWide = false;
		return (const char*)entry->data;
	}
}

bool Game::sigscanFNamesAndAppRealloc() {
	if (attemptedToFindFNameNames) {
		return false;
	}
	attemptedToFindFNameNames = true;
	uintptr_t place = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		"8b 04 81 f6 40 08 01 74 1c 83 c0 10 8d 50 02 eb 03",
		nullptr, "FNameNames");
	if (!place) return false;
	fnameNamesPtr = *(ArrayOfFNameEntryPointers**)(place - 4);
	if (!fnameNamesPtr) return false;
	uintptr_t appReallocCallPlace = sigscanForward(place,
		"6a 08 8d 14 00 52 51 >e8 ?? ?? ?? ?? 83 c4 0c 89 06", 0xc0);
	if (appReallocCallPlace) {
		appRealloc = (appRealloc_t)followRelativeCall(appReallocCallPlace);
	}
	if (!appRealloc) return false;
	if (!detouring.isInTransaction()) finishedSigscanning();
	return true;
}

bool Game::swapOutFPS() {
	if (attemptedToSwapOutFPS) return false;
	attemptedToSwapOutFPS = true;
	
	uintptr_t fpsUsage = sigscanOffset(GUILTY_GEAR_XRD_EXE,
		"f3 0f 10 86 00 05 00 00 0f 2f c1 77 15 f3 0f 10 05 >?? ?? ?? ?? 0f 2f c1 76 08 f3 0f 11 4c 24 08 eb 06",
		nullptr, "fpsUsage");
	if (!detouring.isInTransaction()) finishedSigscanning();
	if (fpsUsage) {
		DWORD newFpsAddr = (DWORD)(uintptr_t)&gifMode.fpsApplied;
		std::vector<char> newBytes(4);
		memcpy(newBytes.data(), &newFpsAddr, 4);
		detouring.patchPlace(fpsUsage, newBytes);
		return true;
	}
	return false;
}

void Game::onFPSChanged() {
	if (gifMode.fpsApplied != 60.F) {
		swapOutFPS();
	}
}

void Game::allowTickForActor(void* actor) {
	if (!actor) return;
	for (void* elem : actorsToAllowTickFor) {
		if (elem == actor) {
			return;
		}
	}
	actorsToAllowTickFor.push_back(actor);
}

// first check if the game mode is network at all, and that you're in a match (although it would also tell if you're just in the lobby)
bool Game::isOnlineTrainingMode_Part() const {
	if (netplayStruct && *netplayStruct) {
		return *(BYTE*)(
			*netplayStruct + 0x3c8 + 0x53
		) == 2;
	}
	return false;
}

uintptr_t Game::findSelectedCharaLocation() {
	static bool sigscannedSelectedCharaLocation = false;
	static uintptr_t selectedCharaLocation = 0;
	if (sigscannedSelectedCharaLocation) return selectedCharaLocation;
	sigscannedSelectedCharaLocation = true;
	
	uintptr_t stringLoc = sigscanStrOffset(
		"GuiltyGearXrd.exe:.rdata",
		"UI_Profile_Chr%02d",
		nullptr,
		"SelectedCharaLocationLighthouse",
		nullptr);
	if (!stringLoc) return 0;
	std::vector<char> sig(6);
	std::vector<char> mask(6);
	std::vector<char> maskForCaching;
	byteSpecificationToSigMask("68 rel(?? ?? ?? ??)", sig, mask, nullptr, 0, &maskForCaching);
	substituteWildcard(sig.data(), mask.data(), 0, (void*)stringLoc);
	uintptr_t codeLoc = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		sig.data(), mask.data(),
		nullptr,
		"SelectedCharaLocationCode",
		maskForCaching.data());
	if (!codeLoc) return 0;
	uintptr_t finalPlace = sigscanBackwards(codeLoc, "e8 ?? ?? ?? ?? 0f be 90 8a 00 00 00", 30);
	if (!finalPlace) return 0;
	uintptr_t funcAddr = followRelativeCall(finalPlace);
	if (*(BYTE*)funcAddr == 0xb8  // MOV EAX,????
			&& *(BYTE*)(funcAddr + 5) == 0xc3) {  // RET
		finishedSigscanning();
		selectedCharaLocation = *(uintptr_t*)(funcAddr + 1);
		return selectedCharaLocation;
	}
	return 0;
}

saveCharaFunc_t Game::findSaveCharaFunc() {
	static bool triedScanning = false;
	static saveCharaFunc_t lastResult = 0;
	if (triedScanning) return lastResult;
	triedScanning = true;
	
	const wchar_t REDGfxMoviePlayer_MenuCharaSelectBaseString[] = L"REDGfxMoviePlayer_MenuCharaSelectBase";
	uintptr_t stringLoc = sigscan("GuiltyGearXrd.exe:.rdata", (const char*)REDGfxMoviePlayer_MenuCharaSelectBaseString,
		sizeof REDGfxMoviePlayer_MenuCharaSelectBaseString, "REDGfxMoviePlayer_MenuCharaSelectBaseString", nullptr);
	if (!stringLoc) return 0;
	
	std::vector<char> sig(6);
	std::vector<char> mask(6);
	std::vector<char> maskForCaching;
	byteSpecificationToSigMask("68 rel(?? ?? ?? ??)", sig, mask, nullptr, 0, &maskForCaching);
	substituteWildcard(sig.data(), mask.data(), 0, (void*)stringLoc);
	uintptr_t privateStaticClassInitialization = sigscanOffset(
		GUILTY_GEAR_XRD_EXE,
		sig.data(), mask.data(),
		nullptr,
		"REDGfxMoviePlayer_MenuCharaSelectBase_PrivateStaticClassInitialization",
		maskForCaching.data());
	if (!privateStaticClassInitialization) return 0;
	
	uintptr_t internalConstructorUsage = sigscanBackwards(privateStaticClassInitialization,
		"68 ?? ?? ?? ?? 68 84 40 08 04", 30);
	if (!internalConstructorUsage) return 0;
	
	uintptr_t internalConstructor = *(uintptr_t*)(internalConstructorUsage + 1);
	
	uintptr_t vtableAssignment = sigscanForward(internalConstructor, "c7 06", 0x20);  // MOV dword ptr [ESI],????
	if (!vtableAssignment) return 0;
	
	lastResult = (saveCharaFunc_t)*(uintptr_t*)(
		*(uintptr_t*)(
			vtableAssignment + 2  // read vtable itself
		) + 0x380  // Network_SetMyChara
	);
	return lastResult;
	
}
