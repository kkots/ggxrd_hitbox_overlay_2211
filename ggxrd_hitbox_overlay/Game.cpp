#include "pch.h"
#include "Game.h"
#include "memoryFunctions.h"

const char** aswEngine = nullptr;

Game game;

bool Game::onDllMain() {
	bool error = false;

	aswEngine = (const char**)sigscanOffset("GuiltyGearXrd.exe",
		"\x85\xC0\x78\x74\x83\xF8\x01",
		{-4, 0},
		&error, "aswEngine");

	gameDataPtr = (const char**)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x33\xC0\x38\x41\x44\x0F\x95\xC0\xC3\xCC",
		{-4, 0},
		NULL, "gameDataPtr");

	return !error;
}

GameMode Game::getGameMode() const {
	if (!gameDataPtr) return GAME_MODE_TRAINING;
	return (GameMode)*(*gameDataPtr + 0x45);
}
