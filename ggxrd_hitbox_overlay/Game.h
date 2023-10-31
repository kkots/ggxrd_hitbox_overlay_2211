#pragma once
#include "gameModes.h"

extern const char** aswEngine;

class Game {
public:
	bool onDllMain();
	GameMode getGameMode() const;
	char getPlayerSide() const;
	bool isMatchRunning() const;
private:
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
};

extern Game game;
