#pragma once
// I'm trying to legalize hardcode
#define MAGICIAN_FRAMEBAR_ID 83
#define KINOMI_OBJ_NECRO_FRAMEBAR_ID 95
#define GAMMA_RAY_LASER_FRAMEBAR_ID 98
#define ANSWER_RSF_FRAMEBAR_ID 106

#define PROJECTILE_NAME_BACCHUS "Bacchus"
#define PROJECTILE_NAME_BERRY "Berry"
#define PROJECTILE_NAME_GHOST "Ghost"
#define MOVE_NAME_NOTE "Note"

// all hits originating from projectiles with these framebar IDs must be combined
inline bool combineHitsFromFramebarId(int framebarId) {
	return framebarId == MAGICIAN_FRAMEBAR_ID
		|| framebarId == KINOMI_OBJ_NECRO_FRAMEBAR_ID
		|| framebarId == GAMMA_RAY_LASER_FRAMEBAR_ID
		|| framebarId == ANSWER_RSF_FRAMEBAR_ID;
}

// Venom Red Hail and Bishop Runout must be hardcoded separately
