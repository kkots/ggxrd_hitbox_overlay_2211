#pragma once

// Find "GAME_MODE_DEBUG_BATTLE" string in the game's executable. It is used in a string table and indices of that table are this enum.
enum GameMode : char {
	/* 0 */    GAME_MODE_DEBUG_BATTLE,
	/* 1 */    GAME_MODE_ADVERTISE,
	/* 2 */    GAME_MODE_ARCADE,
	/* 3 */    GAME_MODE_MOM,
	/* 4 */    GAME_MODE_SPARRING,
	/* 5 */    GAME_MODE_VERSUS,
	/* 6 */    GAME_MODE_TRAINING,
	/* 7 */    GAME_MODE_RANNYU_VERSUS,
	/* 8 */    GAME_MODE_EVENT,
	/* 9 */    GAME_MODE_STORY,
	/* 10 */   GAME_MODE_DEGITALFIGURE,
	/* 11 */   GAME_MODE_MAINMENU,
	/* 12 */   GAME_MODE_TUTORIAL,
	/* 13 */   GAME_MODE_CHALLENGE,
	/* 14 */   GAME_MODE_KENTEI,
	/* 15 */   GAME_MODE_GALLERY,
	/* 16 */   GAME_MODE_NETWORK,
	/* 17 */   GAME_MODE_REPLAY,
	/* 18 */   GAME_MODE_FISHING,
	/* 19 */   GAME_MODE_UNDECIDED,
	/* 20 */   GAME_MODE_INVALID
};
