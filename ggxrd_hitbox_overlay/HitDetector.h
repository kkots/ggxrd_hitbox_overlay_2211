#pragma once
#include "DrawHitboxArrayCallParams.h"
#include <vector>
#include "Entity.h"

using hitDetection_t = BOOL(__thiscall*)(void*, void*, int, int, int*, int*);

class HitDetector
{
public:
	bool onDllMain();
	void clearAllBoxes();
	void drawDetected();
	hitDetection_t orig_hitDetection;
private:
	
	struct DetectedHitboxes {
		Entity attacker;
		int attackerTeam;
		DrawHitboxArrayCallParams hitboxes;
		int counter;
	};

	class HookHelp {
	private:
		friend class HitDetector;
		BOOL hitDetectionHook(void* defender, int attackerHitboxIndex, int defenderHitboxIndex, int* intersectionX, int* intersectionY);
	};

	std::vector<DetectedHitboxes> hitboxesThatHit;

	BOOL hitDetectionHook(void* defender, int attackerHitboxIndex, int defenderHitboxIndex, int* intersectionX, int* intersectionY);
};

extern HitDetector hitDetector;
