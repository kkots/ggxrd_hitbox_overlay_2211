#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "Settings.h"
#include "EndScene.h"
#include "Camera.h"

const char** aswEngine = nullptr;

Game game;

bool Game::onDllMain() {
	bool error = false;

	char aswEngineSig[] = "\x85\xC0\x78\x74\x83\xF8\x01";
	// ghidra sig: 85 C0 78 74 83 F8 01
	aswEngine = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		aswEngineSig,
		_countof(aswEngineSig),
		{-4, 0},
		&error, "aswEngine");

	char gameDataPtrSig[] = "\x33\xC0\x38\x41\x44\x0F\x95\xC0\xC3\xCC";
	// ghidra sig: 33 C0 38 41 44 0F 95 C0 C3 CC
	gameDataPtr = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		gameDataPtrSig,
		_countof(gameDataPtrSig),
		{-4, 0},
		NULL, "gameDataPtr");

	// ghidra sig: 8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? 3c 10 75 10 a1 ?? ?? ?? ?? 85 c0 74 07 c6 80 0c 12 00 00 01 c3
	playerSideNetworkHolder = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x8b\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x3c\x10\x75\x10\xa1\x00\x00\x00\x00\x85\xc0\x74\x07\xc6\x80\x0c\x12\x00\x00\x01\xc3",
		"xx????x????xxxxx????xxxxxxxxxxxx",
		{16, 0},
		NULL, "playerSideNetworkHolder");


	if (!error && sigscanFrameByFraming()) {
		hookFrameByFraming();
	}



	return !error;
}

bool Game::sigscanFrameByFraming() {
	bool error = false;

	// levelTick is called by ULevel::Tick which is called by UGameEngine::Tick
	// ghidra sig: 83 ec 18 53 55 57 8b 7c 24 28 8b 87 48 01 00 00 33 db 89 9f 44 01 00 00 3b c3 7d 22 8b 87 40 01 00 00 89 9f 48 01 00 00 3b c3 74 12 6a 08
	orig_levelTick = (levelTick_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x83\xec\x18\x53\x55\x57\x8b\x7c\x24\x28\x8b\x87\x48\x01\x00\x00\x33\xdb\x89\x9f\x44\x01\x00\x00\x3b\xc3\x7d\x22\x8b\x87\x40\x01\x00\x00\x89\x9f\x48\x01\x00\x00\x3b\xc3\x74\x12\x6a\x08",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		&error, "levelTick");
	
	// ghidra sig: b9 ?? ?? ?? ?? e8 ?? ?? ?? ?? 84 c0 75 0c e8 ?? ?? ?? ?? 8b c8 e8 ?? ?? ?? ?? 8b 4c 24 1c
	uintptr_t trainingHudCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"\xb9\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x84\xc0\x75\x0c\xe8\x00\x00\x00\x00\x8b\xc8\xe8\x00\x00\x00\x00\x8b\x4c\x24\x1c",
		"x????x????xxxxx????xxx????xxxx",
		&error, "trainingHudCallPlace");
	if (trainingHudCallPlace) {
		getTrainingHudArgument = (getTrainingHudArgument_t)followRelativeCall(trainingHudCallPlace + 14);
		trainingHudTick = (trainingHudTick_t)followRelativeCall(trainingHudCallPlace + 21);
	}

	// updateBattleOfflineVer is the offline variant called from AREDGameInfo_Battle::UpdateBattle at E61240 (game version 2211, netcode version 2.15)
	// (it's in the non-else branch of the if ... == 0)
	// ghidra sig: 89 7c 24 14 e8 ?? ?? ?? ?? 85 c0 74 0a 6a 01 e8 ?? ?? ?? ?? 83 c4 04 8b 0d ?? ?? ?? ?? 8b 81 ?? ?? ?? ?? 8b 80 7c 03 00 00
	char updateBattleOfflineVerSig[] = "\x89\x7c\x24\x14\xe8\x00\x00\x00\x00\x85\xc0\x74\x0a\x6a\x01\xe8\x00\x00\x00\x00\x83\xc4\x04\x8b\x0d\x00\x00\x00\x00\x8b\x81\x00\x00\x00\x00\x8b\x80\x7c\x03\x00\x00";
	char updateBattleOfflineVerSigMask[] = "xxxxx????xxxxxxx????xxxxx????xx????xxxxxx";
	substituteWildcard(updateBattleOfflineVerSigMask, updateBattleOfflineVerSig, (char*)&aswEngine, 4, 2);
	orig_updateBattleOfflineVer = (updateBattleOfflineVer_t)((sigscanOffset(
		"GuiltyGearXrd.exe",
		updateBattleOfflineVerSig,
		updateBattleOfflineVerSigMask,
		&error, "updateBattleOfflineVer") - 0x30) & 0xFFFFFFF0);
	logwrap(fprintf(logfile, "Final location of updateBattleOfflineVer: %p\n", orig_updateBattleOfflineVer));

	// ghidra sig: 57 bf 01 00 00 00 39 7c 24 20 75 18 8b 06 8b 90 ec 01 00 00 8b ce ff d2 c7 44 24 10 00 00 00 00
	orig_updateAnimations = (updateAnimations_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x57\xbf\x01\x00\x00\x00\x39\x7c\x24\x20\x75\x18\x8b\x06\x8b\x90\xec\x01\x00\x00\x8b\xce\xff\xd2\xc7\x44\x24\x10\x00\x00\x00\x00",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		{ -8 },
		&error, "updateAnimations");

	return !error;
}

void Game::hookFrameByFraming() {
	
	detouring.attach(&(PVOID&)(orig_levelTick),
		Game::levelTickHookStatic,
		&orig_levelTickMutex,
		"levelTick");

	void(HookHelp::*updateBattleOfflineVerHookPtr)(int param1) = &HookHelp::updateBattleOfflineVerHook;
	detouring.attach(&(PVOID&)orig_updateBattleOfflineVer,
		(PVOID&)updateBattleOfflineVerHookPtr,
		&orig_updateBattleOfflineVerMutex,
		"updateBattleOfflineVer");

	detouring.attach(&(PVOID&)(orig_updateAnimations),
		Game::updateAnimationsHookStatic,
		&orig_updateAnimationsMutex,
		"updateAnimations");

}

void Game::updateAnimationsHookStatic(int param1, int param2, int param3, int param4) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("updateAnimations", true);
		}
		~HookTracker() {
			--detouring.hooksCounter;
			detouring.markHookRunning("updateAnimations", false);
		}
	} hookTracker;
	game.updateAnimationsHook(param1, param2, param3, param4);
}

void Game::updateAnimationsHook(int param1, int param2, int param3, int param4) {
	if (ignoreAllCalls) {
		if (needToCallEndSceneLogic) {
			endScene.logic();
			needToCallEndSceneLogic = false;
		}
		return;
	}
	{
		std::unique_lock<std::mutex> guard(orig_updateAnimationsMutex);
		orig_updateAnimations(param1, param2, param3, param4);
	}
}

void Game::HookHelp::updateBattleOfflineVerHook(int param1) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("updateBattleOfflineVer", true);
		}
		~HookTracker() {
			--detouring.hooksCounter;
			detouring.markHookRunning("updateBattleOfflineVer", false);
		}
	} hookTracker;
	return game.updateBattleOfflineVerHook((char*)this, param1);
}

void Game::updateBattleOfflineVerHook(char* thisArg, int param1) {
	if (ignoreAllCalls) {
		return;
	}
	endScene.assignNextId();
	{
		std::unique_lock<std::mutex> guard(orig_updateBattleOfflineVerMutex);
		orig_updateBattleOfflineVer(thisArg, param1);
	}
}

void Game::levelTickHookStatic(int param1, int param2, int param3, int param4) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("levelTick", true);
		}
		~HookTracker() {
			--detouring.hooksCounter;
			detouring.markHookRunning("levelTick", false);
		}
	} hookTracker;
	game.levelTickHook(param1, param2, param3, param4);
}

void Game::levelTickHook(int param1, int param2, int param3, int param4) {
	// Approximate order in which things get called:
	// 1) This hook gets called in the game logic thread
	// 2) The hooked function calls updateBattleOfflineVerHook
	// 3) After updateBattleOfflineVerHook ends, updateAnimationsHook gets called many times in one frame
	// 4) This hook ends
	// 5) updateAnimationsHook gets called some more times
	// 6) sendUnrealPawnDataHook in EndScene.cpp fires off many times, among which is the one entity that it tracks (once per frame), and the boxes data gets prepared
	// 6.5) Meanwhile, in GUI thread, readUnrealPawnDataHook in EndScene.cpp gets called many times, and reads that data in one of the calls
	// 7) The camera hook gets called in the game logic thread which prepares the camera data for this frame. The problem is that some unknown functions send and receive data for it.
	//    It is certain that sendUnrealPawnDataHook/readUnrealPawnDataHook do not handle the camera data.
	//    We only know for sure that by the time EndScene is called, the camera data is already transferred to GUI thread.

	// In rollback-affected online matches, the camera code runs multiple times per frame, from oldest frame to latest.
	// However, the pawn data is always sent only once per frame.
	// It is also certain that in rollback-affected matches, camera code runs first, and only then sendUnrealPawnDataHook/readUnrealPawnDataHook happen.
	// 
	// When game pause menu is open in single player, sendUnrealPawnDataHook does not get called, which means the mod can't process hotkeys.

	ignoreAllCalls = false;
	needToCallEndSceneLogic = false;
	endScene.butDontPrepareBoxData = false;
	camera.butDontPrepareBoxData = false;
	if (freezeGame && *aswEngine) {
		slowmoSkipCounter = 0;
		if (!allowNextFrame) {
			ignoreAllCalls = true;
		}
		allowNextFrame = false;
	}
	if (slowmoGame && *aswEngine) {
		++slowmoSkipCounter;
		if ((int)slowmoSkipCounter < settings.slowmoTimes) {
			ignoreAllCalls = true;
		} else {
			slowmoSkipCounter = 0;
		}
	} else {
		slowmoSkipCounter = 0;
	}
	if (ignoreAllCalls) {
		needToCallEndSceneLogic = true;
		endScene.butDontPrepareBoxData = true;
		camera.butDontPrepareBoxData = true;
	}
	{
		std::unique_lock<std::mutex> guard(orig_levelTickMutex);
		orig_levelTick(param1, param2, param3, param4);
	}
	if (ignoreAllCalls) {
		levelTickHookEmpty();
	}
}

void Game::levelTickHookEmpty() {
	if (getTrainingHudArgument) {
		trainingHudTick(getTrainingHudArgument());
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
	if (!aswEngine) return false;
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
