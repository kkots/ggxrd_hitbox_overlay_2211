#pragma once
#include "DrawHitboxArrayCallParams.h"
#include <vector>
#include "Entity.h"
#include "PlayerInfo.h"

using determineHitType_t = HitResult(__thiscall*)(void*, void*, BOOL, DWORD*, int*);
using copyDealtAtkToReceivedAtk_t = void(__thiscall*)(void*, void*);
using dealHit_t = void(__thiscall*)(void*, void*, BOOL);

class HitDetector
{
public:
	
	struct WasHitInfo {
		bool wasHit = false;
		DrawHitboxArrayCallParams hurtbox;
	};

	bool onDllMain();
	void clearAllBoxes();
	// may delete items from endScene.attackHitboxes
	void prepareDrawHits();
	void drawHits();
	WasHitInfo wasThisHitPreviously(Entity ent, const DrawHitboxArrayCallParams& currentHurtbox);
	bool hasHitboxThatHit(Entity ent) const;
	determineHitType_t orig_determineHitType = nullptr;
	copyDealtAtkToReceivedAtk_t orig_copyDealtAtkToReceivedAtk = nullptr;
	dealHit_t orig_dealHit = nullptr;
	uintptr_t activeFrameHit = 0;
private:
	
	struct DetectedHitboxes {
		Entity entity{nullptr};
		bool isPawn = false;
		int team = 0;
		DrawHitboxArrayCallParams hitboxes;
		int activeTime = 0;  // this is needed for Chipp's Gamma Blade, it stops being active on the frame after it hits
		int counter = 0;
		unsigned int previousTime = 0;
		int hitboxesCount = 0;
		bool timeHasChanged(bool globalTimeHasChanged);
		bool entityInTheList = false;
		bool entityInTheListAndActive = false;
	};

	class HookHelp {
	private:
		friend class HitDetector;
		HitResult determineHitTypeHook(void* defender, BOOL wasItType10Hitbox, DWORD* hitFlags, int* hpPtr);
		void copyDealtAtkToReceivedAtkHook(void* defender);
		void dealHitHook(void* attacker, BOOL isInHitstun);
	};

	struct Rejection {
		Entity owner{nullptr};
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		int counter = 0;
		int skipFrame = 0;
		int activeFrame = 0;
		bool hatched = false;
		int originX = 0;
		int originY = 0;
		bool firstFrame = false;
	};

	std::vector<DetectedHitboxes> hitboxesThatHit;
	std::vector<DetectedHitboxes> hurtboxesThatGotHit;
	std::vector<Rejection> rejections;

	unsigned int previousTime = 0;
	static bool isMadeFullInvul(Entity ent);
	bool timeHasChanged = false;
};

extern HitDetector hitDetector;
