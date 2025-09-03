#pragma once
#include <vector>
#include "ThrowInfo.h"
#include "HitDetectionType.h"

using hitDetectionMain_t = void (__thiscall*)(void* aswSubengine, HitDetectionType hitDetectionType);
using isActive_t = int(__thiscall*)(void* pawn, BOOL alternativeIsActiveFramesCheck);

class Throws
{
public:
	bool onDllMain();
	void drawThrows();
	void clearAllBoxes();
	int hitDetectionIsActiveHook(void* pawn, BOOL alternativeIsActiveFramesCheck);
private:
	class HookHelp {
	private:
		friend class Throws;
		void hitDetectionMainHook(HitDetectionType hitDetectionType);
		int hitDetectionIsActiveHook(BOOL alternativeIsActiveFramesCheck);
		BOOL clashHitDetectionCallHook(void* defender, HitboxType hitboxIndex, HitboxType defenderHitboxIndex, int* intersectionXPtr, int* intersectionYPtr);
	};

	hitDetectionMain_t orig_hitDetectionMain = nullptr;

	void hitDetectionMainHook();

	std::vector<ThrowInfo> infos;

	unsigned int previousTime = 0;
	
	isActive_t isActivePtr = nullptr;
};

extern Throws throws;
