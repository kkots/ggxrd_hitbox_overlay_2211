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
	void drawHitsInside();
	WasHitInfo wasThisHitPreviously(Entity ent, const DrawHitboxArrayCallParams& currentHurtbox);
	bool hasHitboxThatHit(Entity ent) const;
	determineHitType_t orig_determineHitType = nullptr;
	copyDealtAtkToReceivedAtk_t orig_copyDealtAtkToReceivedAtk = nullptr;
	dealHit_t orig_dealHit = nullptr;
	uintptr_t activeFrameHit = 0;
private:

	class HookHelp {
	private:
		friend class HitDetector;
		HitResult determineHitTypeHook(void* defender, BOOL wasItType10Hitbox, DWORD* hitFlags, int* hpPtr);
		void copyDealtAtkToReceivedAtkHook(void* defender);
		void dealHitHook(void* attacker, BOOL isInHitstun);
	};
	static bool isMadeFullInvul(Entity ent);
};

extern HitDetector hitDetector;
