#pragma once

// Find "GAME_MODE_DEBUG_BATTLE" string in the game's executable. It is used in a string table and indices of that table are this enum.
enum GameMode : char {
	/* 0 */    GAME_MODE_DEBUG_BATTLE,
	/* 1 */    GAME_MODE_ADVERTISE,
	/* 2 */    GAME_MODE_ARCADE,  // story mode
	/* 3 */    GAME_MODE_MOM,  // mom mode
	/* 4 */    GAME_MODE_SPARRING,  // idk
	/* 5 */    GAME_MODE_VERSUS,  // versus mode
	/* 6 */    GAME_MODE_TRAINING,  // training mode
	/* 7 */    GAME_MODE_RANNYU_VERSUS,
	/* 8 */    GAME_MODE_EVENT,
	/* 9 */    GAME_MODE_STORY,  // not a battle mode. This is the mode that plays story cutscenes
	/* 10 */   GAME_MODE_DEGITALFIGURE,
	/* 11 */   GAME_MODE_MAINMENU,  // main menu. But if you go to character select screen it already becomes a different mode
	/* 12 */   GAME_MODE_TUTORIAL,  // tutorial mode
	/* 13 */   GAME_MODE_CHALLENGE,  // combo mode
	/* 14 */   GAME_MODE_KENTEI,  // mission mode
	/* 15 */   GAME_MODE_GALLERY,
	/* 16 */   GAME_MODE_NETWORK,  // online observer or direct participant, ranked, online training mode
	/* 17 */   GAME_MODE_REPLAY,  // replay mode
	/* 18 */   GAME_MODE_FISHING,
	/* 19 */   GAME_MODE_UNDECIDED,
	/* 20 */   GAME_MODE_INVALID
};
