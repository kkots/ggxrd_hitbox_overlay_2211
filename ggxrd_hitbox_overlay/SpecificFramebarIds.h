#pragma once
// I'm trying to legalize hardcode
// can't use __COUNTER__ in a header file: produces a different number in each translation unit
// let's instead hardcode a number that __COUNTER__ will never reach
#define MAGICIAN_FRAMEBAR_ID 0x70000001
#define KINOMI_OBJ_NECRO_FRAMEBAR_ID 0x70000002
#define GAMMA_RAY_LASER_FRAMEBAR_ID 0x70000003
#define ANSWER_RSF_FRAMEBAR_ID 0x70000004

#define PROJECTILE_NAME_BACCHUS "Bacchus"
#define PROJECTILE_NAME_BERRY "Berry"
#define PROJECTILE_NAME_GHOST "Ghost"
#define PROJECTILE_NAME_TUNING_BALL "Tuning Ball"
#define PROJECTILE_NAME_CELESTIAL_TUNING_BALL "Celestial Tuning Ball"
#define MOVE_NAME_NOTE "Note"

// all hits originating from projectiles with these framebar IDs must be combined
inline bool combineHitsFromFramebarId(int framebarId) {
	return framebarId == MAGICIAN_FRAMEBAR_ID
		|| framebarId == KINOMI_OBJ_NECRO_FRAMEBAR_ID
		|| framebarId == GAMMA_RAY_LASER_FRAMEBAR_ID
		|| framebarId == ANSWER_RSF_FRAMEBAR_ID;
}

// Venom Red Hail and Bishop Runout must be hardcoded separately
