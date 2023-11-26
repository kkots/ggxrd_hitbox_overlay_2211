#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "Settings.h"
#include "EndScene.h"

const char** aswEngine = nullptr;

Game game;

bool Game::onDllMain() {
	bool error = false;

	char aswEngineSig[] = "\x85\xC0\x78\x74\x83\xF8\x01";
	aswEngine = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		aswEngineSig,
		_countof(aswEngineSig),
		{-4, 0},
		&error, "aswEngine");

	char gameDataPtrSig[] = "\x33\xC0\x38\x41\x44\x0F\x95\xC0\xC3\xCC";
	gameDataPtr = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		gameDataPtrSig,
		_countof(gameDataPtrSig),
		{-4, 0},
		NULL, "gameDataPtr");

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

	orig_gameLoop = (gameLoop_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x83\xec\x18\x53\x55\x57\x8b\x7c\x24\x28\x8b\x87\x48\x01\x00\x00\x33\xdb\x89\x9f\x44\x01\x00\x00\x3b\xc3\x7d\x22\x8b\x87\x40\x01\x00\x00\x89\x9f\x48\x01\x00\x00\x3b\xc3\x74\x12\x6a\x08",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		&error, "gameLoop");
	
	uintptr_t trainingHudCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"\xb9\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x84\xc0\x75\x0c\xe8\x00\x00\x00\x00\x8b\xc8\xe8\x00\x00\x00\x00\x8b\x4c\x24\x1c",
		"x????x????xxxxx????xxx????xxxx",
		&error, "trainingHudCallPlace");
	if (trainingHudCallPlace) {
		getTrainingHudArgument = (getTrainingHudArgument_t)followRelativeCall(trainingHudCallPlace + 14);
		trainingHudTick = (trainingHudTick_t)followRelativeCall(trainingHudCallPlace + 21);
	}

	char actualGameLoopSig[] = "\x89\x7c\x24\x14\xe8\x00\x00\x00\x00\x85\xc0\x74\x0a\x6a\x01\xe8\x00\x00\x00\x00\x83\xc4\x04\x8b\x0d\x00\x00\x00\x00\x8b\x81\x00\x00\x00\x00\x8b\x80\x7c\x03\x00\x00";
	char actualGameLoopSigMask[] = "xxxxx????xxxxxxx????xxxxx????xx????xxxxxx";
	substituteWildcard(actualGameLoopSigMask, actualGameLoopSig, (char*)&aswEngine, 4, 2);
	orig_actualGameLoop = (actualGameLoop_t)((sigscanOffset(
		"GuiltyGearXrd.exe",
		actualGameLoopSig,
		actualGameLoopSigMask,
		&error, "actualGameLoop") - 0x30) & 0xFFFFFFF0);
	logwrap(fprintf(logfile, "Final location of actualGameLoop: %p\n", orig_actualGameLoop));

	orig_updateAnimations = (updateAnimations_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x57\xbf\x01\x00\x00\x00\x39\x7c\x24\x20\x75\x18\x8b\x06\x8b\x90\xec\x01\x00\x00\x8b\xce\xff\xd2\xc7\x44\x24\x10\x00\x00\x00\x00",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		{ -8 },
		&error, "updateAnimations");

	return !error;
}

void Game::hookFrameByFraming() {
	
	detouring.attach(&(PVOID&)(orig_gameLoop),
		Game::gameLoopHookStatic,
		&orig_gameLoopMutex,
		"gameLoop");

	void(HookHelp::*actualGameLoopHookPtr)(int param1) = &HookHelp::actualGameLoopHook;
	detouring.attach(&(PVOID&)orig_actualGameLoop,
		(PVOID&)actualGameLoopHookPtr,
		&orig_actualGameLoopMutex,
		"actualGameLoop");

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

void Game::HookHelp::actualGameLoopHook(int param1) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("actualGameLoop", true);
		}
		~HookTracker() {
			--detouring.hooksCounter;
			detouring.markHookRunning("actualGameLoop", false);
		}
	} hookTracker;
	return game.actualGameLoopHook((char*)this, param1);
}

void Game::actualGameLoopHook(char* thisArg, int param1) {
	if (ignoreAllCalls) {
		return;
	}
	{
		std::unique_lock<std::mutex> guard(orig_actualGameLoopMutex);
		orig_actualGameLoop(thisArg, param1);
	}
}

void Game::gameLoopHookStatic(int param1, int param2, int param3, int param4) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("gameLoop", true);
		}
		~HookTracker() {
			--detouring.hooksCounter;
			detouring.markHookRunning("gameLoop", false);
		}
	} hookTracker;
	game.gameLoopHook(param1, param2, param3, param4);
}

void Game::gameLoopHook(int param1, int param2, int param3, int param4) {
	ignoreAllCalls = false;
	needToCallEndSceneLogic = false;
	if (freezeGame) {
		slowmoSkipCounter = 0;
		if (!allowNextFrame) {
			ignoreAllCalls = true;
		}
		allowNextFrame = false;
	}
	if (slowmoGame) {
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
	}
	{
		std::unique_lock<std::mutex> guard(orig_gameLoopMutex);
		orig_gameLoop(param1, param2, param3, param4);
	}
	if (ignoreAllCalls) {
		gameLoopHookEmpty();
	}
}

void Game::gameLoopHookEmpty() {
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
