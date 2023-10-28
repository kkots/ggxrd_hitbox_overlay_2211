#pragma once
#include "DrawHitboxArrayCallParams.h"
#include <vector>
#include "Entity.h"

using hitDetection_t = BOOL(__thiscall*)(void*, void*, int, int, int*, int*);

class HitDetector
{
public:
	
	struct WasHitInfo {
		bool wasHit = false;
		DrawHitboxArrayCallParams hurtbox;
		bool counterhit = false;
	};

	bool onDllMain();
	void clearAllBoxes();
	void drawHits();
	WasHitInfo wasThisHitPreviously(Entity ent) const;
	hitDetection_t orig_hitDetection;
private:
	
	struct DetectedHitboxes {
		Entity entity;
		int team;
		DrawHitboxArrayCallParams hitboxes;
		int counter;
		bool counterhit;
	};

	class HookHelp {
	private:
		friend class HitDetector;
		BOOL hitDetectionHook(void* defender, int attackerHitboxIndex, int defenderHitboxIndex, int* intersectionX, int* intersectionY);
	};

	std::vector<DetectedHitboxes> hitboxesThatHit;
	std::vector<DetectedHitboxes> hurtboxesThatGotHit;

	unsigned int previousTime = 0;
};

extern HitDetector hitDetector;
