#pragma once

extern const char** aswEngine;

enum GameMode : char {
	GAME_MODE_DEBUG_BATTLE,
	GAME_MODE_ADVERTISE,
	GAME_MODE_ARCADE,
	GAME_MODE_MOM,
	GAME_MODE_SPARRING,
	GAME_MODE_VERSUS,
	GAME_MODE_TRAINING,
	GAME_MODE_RANNYU_VERSUS,
	GAME_MODE_EVENT,
	GAME_MODE_STORY,
	GAME_MODE_DEGITALFIGURE,
	GAME_MODE_MAINMENU,
	GAME_MODE_TUTORIAL,
	GAME_MODE_CHALLENGE,
	GAME_MODE_KENTEI,
	GAME_MODE_GALLERY,
	GAME_MODE_NETWORK,
	GAME_MODE_REPLAY,
	GAME_MODE_FISHING,
	GAME_MODE_UNDECIDED,
	GAME_MODE_INVALID
};

class Game {
public:
	bool onDllMain();
	GameMode getGameMode() const;
private:
	const char** gameDataPtr = nullptr;
};

extern Game game;
