#pragma once
#include "gameModes.h"

extern const char** aswEngine;

class Game {
public:
	bool onDllMain();
	GameMode getGameMode() const;
	bool currentModeIsOnline() const;
	bool isNonOnline() const;
	char getPlayerSide() const;
	bool isMatchRunning() const;
	bool isTrainingMode() const;
private:
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
};

extern Game game;
