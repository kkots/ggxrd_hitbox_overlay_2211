#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "Settings.h"
#include "EndScene.h"
#include "Camera.h"
#include "GifMode.h"
#include "WinError.h"
#include "EntityList.h"

const char** aswEngine = nullptr;

Game game;

bool Game::onDllMain() {
	bool error = false;

	aswEngine = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"85 C0 78 74 83 F8 01",
		{-4, 0},
		&error, "aswEngine");

	gameDataPtr = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"33 C0 38 41 44 0F 95 C0 C3 CC",
		{-4, 0},
		NULL, "gameDataPtr");

	playerSideNetworkHolder = (const char**)sigscanOffset(
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
			&orig_UWorld_TickMutex,
			"UWorld_Tick");
	}
	
	if (orig_UWorld_IsPaused) {
		bool(HookHelp::*UWorld_IsPausedHookPtr)() = &HookHelp::UWorld_IsPausedHook;
		detouring.attach(&(PVOID&)(orig_UWorld_IsPaused),
			(PVOID&)UWorld_IsPausedHookPtr,
			&orig_UWorld_IsPausedMutex,
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
		substituteWildcard(burstSig.data(), burstMask.data(), 0, aswEngine);
		burstOffset = sigscanOffset(
			"GuiltyGearXrd.exe",
			burstSig.data(),
			burstMask.data(),
			{ 11, 0 },
			NULL, "burstOffset");
		
		std::vector<char> destroySig;
		std::vector<char> destroyMask;
		byteSpecificationToSigMask("c7 05 ?? ?? ?? ?? 00 00 00 00",
			destroySig, destroyMask);
		substituteWildcard(destroySig.data(), destroyMask.data(), 0, aswEngine);
		orig_destroyAswEngine = (destroyAswEngine_t)sigscanOffset(
			"GuiltyGearXrd.exe",
			destroySig.data(),
			destroyMask.data(),
			{ -0x5A },
			NULL, "destroyAswEngine");
	
		// offset from aswEngine to its field containing a pointer to an instance of AREDCamera_Battle class
		std::vector<char> cameraOffsetSig;
		std::vector<char> cameraOffsetSigMask;
		byteSpecificationToSigMask("8b 4c 24 18 83 c4 08 0b 4c 24 14 74 1e 8b 15 ?? ?? ?? ?? 8b 8a ?? ?? ?? ?? e8 ?? ?? ?? ?? 85 c0 75 09 55 e8 ?? ?? ?? ?? 83 c4 04",
			cameraOffsetSig, cameraOffsetSigMask);
	
		substituteWildcard(cameraOffsetSig.data(), cameraOffsetSigMask.data(), 0, aswEngine);
		// pointer to REDCamera_Battle
		cameraOffset = (unsigned int)sigscanOffset(
			"GuiltyGearXrd.exe",
			cameraOffsetSig.data(),
			cameraOffsetSigMask.data(),
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
			&orig_destroyAswEngineMutex,
			"destroyAswEngine");
	}
	
	aswEngineTickCountOffset = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8d bd a4 04 00 00 bd 02 00 00 00 90 8b 0f 3b cb 74 05 e8 ?? ?? ?? ?? 83 c7 04 4d 75 ef ff 86",
		{ 31, 0 },
		&error, "aswEngineTickCountOffset");
	

	std::vector<char> sig;
	std::vector<char> mask;
	if (REDHUD_BattleOffset) {
		byteSpecificationToSigMask("8b 81 ?? ?? ?? ?? f6 80 5c 04 00 00 08 74 14 8b 54 24 14 f6 82 c8 04 00 00 02",
			sig, mask);
		substituteWildcard(sig.data(), mask.data(), 0, (void*)REDHUD_BattleOffset);
		uintptr_t drawExGaugeHUDCallPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			mask.data(),
			nullptr, "drawExGaugeHUDCallPlace");
		if (drawExGaugeHUDCallPlace) {
			drawExGaugeHUDOffset = *(DWORD*)(drawExGaugeHUDCallPlace + 40);
			drawExGaugeHUD = (drawExGaugeHUD_t)followRelativeCall(drawExGaugeHUDCallPlace + 44);
			
			void(HookHelp::*drawExGaugeHUDHookPtr)(int param_1) = &HookHelp::drawExGaugeHUDHook;
			detouring.attach(&(PVOID&)(orig_drawExGaugeHUD),
				(PVOID&)drawExGaugeHUDHookPtr,
				&orig_drawExGaugeHUDMutex,
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
		substituteWildcard(sig.data(), mask.data(), 0, (void*)_JitabataRecover);
		
		stunmashDrawingPlace = sigscanOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			mask.data(),
			{ 0x2b2 },
			nullptr, "stunmashDrawingPlace");
		
	}
	
	if (stunmashDrawingPlace) {
		drawStunMashPtr = (drawStunMash_t)followRelativeCall(stunmashDrawingPlace);
	}
	
	
	return !error;
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
		getTrainingHudArgument = (getTrainingHudArgument_t)followRelativeCall(trainingHudCallPlace + 14);
		trainingHudTick = (trainingHudTick_t)followRelativeCall(trainingHudCallPlace + 21);
	}

	// updateBattleOfflineVer is the offline variant called from AREDGameInfo_Battle::UpdateBattle at E61240 (game version 2211, netcode version 2.15)
	// (it's in the non-else branch of the if ... == 0)
	std::vector<char> updateBattleOfflineVerSig;
	std::vector<char> updateBattleOfflineVerSigMask;
	byteSpecificationToSigMask("89 7c 24 14 e8 ?? ?? ?? ?? 85 c0 74 0a 6a 01 e8 ?? ?? ?? ?? 83 c4 04 8b 0d ?? ?? ?? ?? 8b 81 ?? ?? ?? ?? 8b 80 7c 03 00 00",
		updateBattleOfflineVerSig, updateBattleOfflineVerSigMask);
	substituteWildcard(updateBattleOfflineVerSig.data(), updateBattleOfflineVerSigMask.data(), 2, aswEngine);
	orig_updateBattleOfflineVer = (updateBattleOfflineVer_t)((sigscanOffset(
		"GuiltyGearXrd.exe",
		updateBattleOfflineVerSig.data(),
		updateBattleOfflineVerSigMask.data(),
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
		&orig_TickActors_FDeferredTickList_FGlobalActorIteratorMutex,
		"TickActors_FDeferredTickList_FGlobalActorIterator");

	void(HookHelp::*updateBattleOfflineVerHookPtr)(int param1) = &HookHelp::updateBattleOfflineVerHook;
	detouring.attach(&(PVOID&)orig_updateBattleOfflineVer,
		(PVOID&)updateBattleOfflineVerHookPtr,
		&orig_updateBattleOfflineVerMutex,
		"updateBattleOfflineVer");

	detouring.attach(&(PVOID&)(orig_TickActorComponents),
		Game::TickActorComponentsHookStatic,
		&orig_TickActorComponentsMutex,
		"TickActorComponents");

}

void Game::TickActorComponentsHookStatic(int param1, int param2, int param3, int param4) {
	HookGuard hookGuard("TickActorComponents");
	game.TickActorComponentsHook(param1, param2, param3, param4);
}

void Game::TickActorComponentsHook(int param1, int param2, int param3, int param4) {
	if (!shutdown) {
		if (ignoreAllCalls) {
			if (needToCallEndSceneLogic) {
				endScene.logic();
				needToCallEndSceneLogic = false;
			}
			return;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_TickActorComponentsMutex);
		orig_TickActorComponents(param1, param2, param3, param4);
	}
}

void Game::HookHelp::updateBattleOfflineVerHook(int param1) {
	HookGuard hookGuard("updateBattleOfflineVer");
	return game.updateBattleOfflineVerHook((char*)this, param1);
}

void Game::updateBattleOfflineVerHook(char* thisArg, int param1) {
	if (!shutdown) {
		if (ignoreAllCalls) {
			return;
		}
		endScene.assignNextId();
	}
	{
		std::unique_lock<std::mutex> guard(orig_updateBattleOfflineVerMutex);
		orig_updateBattleOfflineVer(thisArg, param1);
	}
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookStatic(int param1, int param2, int param3, int param4) {
	HookGuard hookGuard("TickActors_FDeferredTickList_FGlobalActorIterator");
	game.TickActors_FDeferredTickList_FGlobalActorIteratorHook(param1, param2, param3, param4);
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHook(int param1, int param2, int param3, int param4) {
	// Approximate order in which things get called:
	// 1) This hook gets called in the game logic thread
	// 2) The hooked function calls updateBattleOfflineVerHook
	// 3) After updateBattleOfflineVerHook ends, TickActorComponentsHook gets called many times in one frame
	// 4) This hook ends
	// 5) TickActorComponentsHook gets called some more times
	// 6) USkeletalMeshComponent_UpdateTransformHook in EndScene.cpp fires off many times, among which is the one entity that it tracks (once per frame), and the boxes data gets prepared
	// 6.5) Meanwhile, in GUI thread, readUnrealPawnDataHook in EndScene.cpp gets called many times, and reads that data in one of the calls
	// 7) The camera hook gets called in the game logic thread which prepares the camera data for this frame. The problem is that some unknown functions send and receive data for it.
	//    It is certain that USkeletalMeshComponent_UpdateTransformHook/readUnrealPawnDataHook do not handle the camera data.
	//    We only know for sure that by the time EndScene is called, the camera data is already transferred to GUI thread.

	// In rollback-affected online matches, the camera code runs multiple times per frame, from oldest frame to latest.
	// However, the pawn data is always sent only once per frame.
	// It is also certain that in rollback-affected matches, camera code runs first, and only then USkeletalMeshComponent_UpdateTransformHook/readUnrealPawnDataHook happen.
	// 
	// When game pause menu is open in single player, USkeletalMeshComponent_UpdateTransformHook does not get called, which means the mod can't process hotkeys.
	if (!shutdown) {
		ignoreAllCalls = ignoreAllCallsButEarlier;
		needToCallEndSceneLogic = ignoreAllCallsButEarlier;
		endScene.butDontPrepareBoxData = ignoreAllCallsButEarlier;
		camera.butDontPrepareBoxData = ignoreAllCallsButEarlier;
	}
	{
		std::unique_lock<std::mutex> guard(orig_TickActors_FDeferredTickList_FGlobalActorIteratorMutex);
		orig_TickActors_FDeferredTickList_FGlobalActorIterator(param1, param2, param3, param4);
	}
	if (!shutdown) {
		if (ignoreAllCalls) {
			TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty();
		}
	}
}

void Game::TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty() {
	if (getTrainingHudArgument) {
		trainingHudTick(getTrainingHudArgument());
	}
	if (drawExGaugeHUD) {
		char* battleHud = *(char**)(*aswEngine + REDHUD_BattleOffset);
		char field1 = *(char*)(battleHud + 0x45c);
		char* gameInfoBattle = *(char**)(*aswEngine + REDGameInfo_BattleOffset);
		char field2 = *(char*)(gameInfoBattle + 0x4c8);
		drawExGaugeHUD((void*)(*aswEngine + drawExGaugeHUDOffset), (field1 & 0x8) != 0 && (field2 & 0x2) != 0);
	}
	entityList.populate();
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
		if (!playerSideNetworkHolder) return 2;
		// Big thanks to WorseThanYou for finding this value
		return *(char*)(*playerSideNetworkHolder + 0x1734);  // 0 for p1 side, 1 for p2 side, 2 for observer
	} else if (gameDataPtr && *gameDataPtr) {
		return *(char*)(*gameDataPtr + 0x44);  // this makes sense for training mode for example (maybe only all single player modes)
	} else {
		return 2;
	}
}

GameMode Game::getGameMode() const {
	if (!gameDataPtr || !(*gameDataPtr)) return GAME_MODE_TRAINING;
	return (GameMode)*(*gameDataPtr + 0x45);
}

bool Game::isMatchRunning() const {
	if (!*aswEngine) return false;
	return *(unsigned int*)(*aswEngine + 4 + 0x1c71f0 + 0x12C) != 0; // thanks to WorseThanYou for finding this
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
	HookGuard hookGuard("destroyAswEngine");
	if (!game.shutdown) {
		if (*aswEngine) {
			logwrap(fputs("Asw Engine destroyed\n", logfile));
			endScene.onAswEngineDestroyed();
		}
	}
	{
		std::unique_lock<std::mutex> guard(game.orig_destroyAswEngineMutex);
		game.orig_destroyAswEngine();
	}
}

void Game::UWorld_TickHook(void* thisArg, ELevelTick TickType, float DeltaSeconds) {
	HookGuard hookGuard("UWorld_Tick");
	IsPausedCallCount = 0;
	
	if (!shutdown) {
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
	}
	
	bool hadAsw = *aswEngine != nullptr;
	endScene.onUWorld_TickBegin();
	{
		std::unique_lock<std::mutex> guard(game.orig_UWorld_TickMutex);
		game.orig_UWorld_Tick(thisArg, TickType, DeltaSeconds);
	}
	endScene.onUWorld_Tick();
	if (!shutdown && !game.orig_destroyAswEngine && hadAsw && *aswEngine == nullptr) {
		logwrap(fputs("Asw Engine destroyed\n", logfile));
		endScene.onAswEngineDestroyed();
	}
	ignoreAllCalls = false;
}

void Game::HookHelp::UWorld_TickHook(ELevelTick TickType, float DeltaSeconds) {
	game.UWorld_TickHook(this, TickType, DeltaSeconds);
}

void Game::HookHelp::drawExGaugeHUDHook(int param_1) {
	HookGuard hookGuard("drawExGaugeHUD");
	if (gifMode.modDisabled || !(gifMode.gifModeOn || gifMode.gifModeToggleHudOnly)) {
		std::unique_lock<std::mutex> guard(game.orig_drawExGaugeHUDMutex);
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
	HookGuard hookGuard("UWorld_IsPaused");
	// This function is called from UWorld::Tick several times.
	
	// If UWorld->DemoRecDriver is not nullptr, we might get an extra call at the start that messes us up.
	// It's a: 'Fake NetDriver for capturing network traffic to record demos'.
	// Probably will never be non-0.
	++game.IsPausedCallCount;
	if (!game.shutdown && game.IsPausedCallCount == 2) {
		if (game.ignoreAllCallsButEarlier) {
			return true;
		}
	}
	std::unique_lock<std::mutex> guard(game.orig_UWorld_IsPausedMutex);
	return game.orig_UWorld_IsPaused((void*)this);
}
