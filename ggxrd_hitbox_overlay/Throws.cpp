#include "pch.h"
#include "Throws.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "EntityList.h"
#include "Entity.h"
#include "Graphics.h"
#include "InvisChipp.h"
#include "logging.h"
#include "colors.h"

Throws throws;

bool Throws::onDllMain() {
	bool error = false;

#ifndef USE_ANOTHER_HOOK
	orig_hitDetectionMain = (hitDetectionMain_t)sigscanOffset("GuiltyGearXrd.exe",
		"\x83\xec\x50\xa1\x00\x00\x00\x00\x33\xc4\x89\x44\x24\x4c\x53\x55\x8b\xd9\x56\x57\x8d\x83\x10\x98\x16\x00\x8d\x54\x24\x1c\x33\xed\x89\x44\x24\x10",
		"xxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		&error, "hitDetectionMain");
#else
	orig_hitDetectionMain = (hitDetectionMain_t)sigscanOffset("GuiltyGearXrd.exe",
		"\x51\x53\x8b\x5c\x24\x0c\x55\x8b\x6b\x10\xf7\xdd\x56\x1b\xed\x23\xeb\x53\x8b\xf1\x89\x6c\x24\x10\xe8\x00\x00\x00\x00\x85\xc0\x74\x09",
		"xxxxxxxxxxxxxxxxxxxxxxxxx????xxxx",
		&error, "hitDetectionMain");
#endif

#ifndef USE_ANOTHER_HOOK
	void (HookHelp::*hookPtr)(int) = &HookHelp::hitDetectionMainHook;
#else
	BOOL (HookHelp:: * hookPtr)(char*) = &HookHelp::hitDetectionMainHook;
#endif
	if (!detouring.attach(&(PVOID&)orig_hitDetectionMain,
		(PVOID&)hookPtr,
		&orig_hitDetectionMainMutex,
		"hitDetectionMain")) return false;

	return !error;
}

#ifndef USE_ANOTHER_HOOK
void Throws::HookHelp::hitDetectionMainHook(int hitDetectionType) {
	++detouring.hooksCounter;
	if (hitDetectionType == 1) {
		throws.hitDetectionMainHook();
	}
	{
		MutexWhichTellsWhatThreadItsLockedByGuard guard(throws.orig_hitDetectionMainMutex);
		throws.orig_hitDetectionMain(this, hitDetectionType);
	}
	--detouring.hooksCounter;
}
#else
BOOL Throws::HookHelp::hitDetectionMainHook(char* other) {
	++detouring.hooksCounter;
	throws.hitDetectionMainHook();
	BOOL result;
	{
		MutexWhichTellsWhatThreadItsLockedByGuard guard(throws.orig_hitDetectionMainMutex);
		result throws.orig_hitDetectionMain((char*)this, other);
	}
	--detouring.hooksCounter;
	return result;
}
#endif

void Throws::hitDetectionMainHook() {
	entityList.populate();
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent{entityList.list[i]};

		const int attackActive = *(const int*)(ent + 0x44C);
		const bool isActive = ent.isActive();
		int throwRange = *(const int*)(ent + 0x494);  // This value resets to -1 on normal throws very fast so that's why we need this separate hook.
		                                              // For command throws it stays non -1 for much longer than the throw actually happens

		const int throwMinX = *(const int*)(ent + 0x48C);
		const int throwMaxX = *(const int*)(ent + 0x484);

		const int throwMaxY = *(const int*)(ent + 0x488);
		const int throwMinY = *(const int*)(ent + 0x490);

		const unsigned int currentAnimDuration = ent.currentAnimDuration();
		CharacterType charType = ent.characterType();
		CharacterType ownerType = (CharacterType)(-1);
		if (charType == -1) {
			ownerType = Entity{entityList.slots[ent.team()]}.characterType();
		}

		bool checkPassed = (throwRange >= 0 || throwMinX < throwMaxX || throwMinY < throwMaxY)
			&& (attackActive == 1  // 1 is more like 4/6 H (+OS possibly) throw
			|| (attackActive == 2 || attackActive == 3)  // 2 is a command throw, 3 is Jack-O super throw
			&& isActive
			&& !(currentAnimDuration > 25 && charType == CHARACTER_TYPE_AXL) // the 25 check is needed to stop Axl from showing a throwbox all throughout his Yes move
			&& ((*(const unsigned int*)(ent + 0x4D28) & 0x40) != 0  // 0x4D28 & 0x40 means the move requires no hitbox vs hurtbox detection
			|| (*(const unsigned int*)(ent + 0x460) & 4) != 0)  // 0x460 & 4 means the move will be "unmissable".
			|| ownerType == CHARACTER_TYPE_AXL);

		if (charType == CHARACTER_TYPE_FAUST
				&& strcmp(ent + 0x2444, "Mettagiri") == 0
				&& currentAnimDuration <= 1) {
			throwRange = 175000;
			checkPassed = true;
		}

		if (checkPassed) {

			const int posX = ent.posX();
			const int flip = ent.isFacingLeft() ? -1 : 1;

			ThrowInfo throwInfo;
			throwInfo.active = true;
			throwInfo.owner = ent;
			throwInfo.framesLeft = DISPLAY_DURATION_THROW;
			throwInfo.leftUnlimited = true;
			throwInfo.rightUnlimited = true;

			if (throwRange >= 0) {
				int otherLeft;
				int otherRight;

				Entity other{ nullptr };
				if (entityList.count >= 2) {
					other.ent = (const char*)(entityList.slots[1 - ent.team()]);
				} else {
					other.ent = entityList.slots[0];
				}
				other.pushboxLeftRight(&otherLeft, &otherRight);
				int otherX = other.posX();

				throwInfo.leftUnlimited = false;
				throwInfo.rightUnlimited = false;
				ent.pushboxLeftRight(&throwInfo.left, &throwInfo.right);
				throwInfo.left -= throwRange + (otherRight - otherX);
				throwInfo.right += throwRange + (otherX - otherLeft);
			}

			if (throwMinX < throwMaxX) {
				int throwMinXInSpace = posX + throwMinX * flip;
				int throwMaxXInSpace = posX + throwMaxX * flip;
				
				if (throwMinXInSpace > throwMaxXInSpace) {
					std::swap(throwMinXInSpace, throwMaxXInSpace);
				}

				if (throwInfo.leftUnlimited || throwMinXInSpace > throwInfo.left) throwInfo.left = throwMinXInSpace;
				if (throwInfo.rightUnlimited || throwMaxXInSpace < throwInfo.right) throwInfo.right = throwMaxXInSpace;
				throwInfo.leftUnlimited = false;
				throwInfo.rightUnlimited = false;
			}

			if (throwMinY < throwMaxY) {
				const int posY = ent.posY();
				const int throwMinYInSpace = posY + throwMinY;
				const int throwMaxYInSpace = posY + throwMaxY;

				throwInfo.topUnlimited = false;
				throwInfo.bottomUnlimited = false;

				throwInfo.top = throwMaxYInSpace;
				throwInfo.bottom = throwMinYInSpace;
			}

			for (auto it = infos.begin(); it != infos.end(); ++it) {
				if (it->owner == throwInfo.owner) {
					infos.erase(it);
					break;
				}
			}
			infos.push_back(throwInfo);

		}
	}
}

void Throws::drawThrows() {
	bool timeHasChanged = false;
	unsigned int currentTime = entityList.getCurrentTime();
	if (previousTime != currentTime) {
		previousTime = currentTime;
		timeHasChanged = true;
	}

	auto it = infos.begin();
	while (it != infos.end()) {
		ThrowInfo& throwInfo = *it;

		if (invisChipp.needToHide(throwInfo.owner)) {
			it = infos.erase(it);
			continue;
		}

		if (timeHasChanged && !throwInfo.firstFrame) {
			throwInfo.active = false;
			--throwInfo.framesLeft;
			if (throwInfo.framesLeft <= 0) {
				it = infos.erase(it);
				continue;
			}
		}
		throwInfo.firstFrame = false;

		DrawBoxCallParams params;
		params.fillColor = replaceAlpha(throwInfo.active ? 64 : 0, COLOR_THROW);
		params.outlineColor = replaceAlpha(255, COLOR_THROW);
		params.thickness = THICKNESS_THROW;

		if (throwInfo.leftUnlimited) params.left = -10000000;
		else params.left = throwInfo.left;
		if (throwInfo.rightUnlimited) params.right = 10000000;
		else params.right = throwInfo.right;
		if (throwInfo.bottomUnlimited) params.bottom = -10000000;
		else params.bottom = throwInfo.bottom;
		if (throwInfo.topUnlimited) params.top = 10000000;
		else params.top = throwInfo.top;

		graphics.throwBoxes.push_back(params);
		++it;
	}
}
