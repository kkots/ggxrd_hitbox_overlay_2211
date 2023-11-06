#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"

const char** aswEngine = nullptr;

Game game;

bool Game::onDllMain() {
	bool error = false;

	char aswEngineSig[] = "\x85\xC0\x78\x74\x83\xF8\x01";
	aswEngine = (const char**)sigscanOffset("GuiltyGearXrd.exe",
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

	return !error;
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
