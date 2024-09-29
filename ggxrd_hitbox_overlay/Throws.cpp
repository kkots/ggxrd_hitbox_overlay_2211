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
#include "GifMode.h"
#include "Settings.h"
#include "EndScene.h"
#include "Game.h"

Throws throws;

bool Throws::onDllMain() {
	bool error = false;

#ifndef USE_ANOTHER_HOOK
	orig_hitDetectionMain = (hitDetectionMain_t)sigscanOffset("GuiltyGearXrd.exe",
		"83 ec 50 a1 ?? ?? ?? ?? 33 c4 89 44 24 4c 53 55 8b d9 56 57 8d 83 10 98 16 00 8d 54 24 1c 33 ed 89 44 24 10",
		&error, "hitDetectionMain");
#else
	// ghidra sig: 
	orig_hitDetectionMain = (hitDetectionMain_t)sigscanOffset("GuiltyGearXrd.exe",
		"51 53 8b 5c 24 0c 55 8b 6b 10 f7 dd 56 1b ed 23 eb 53 8b f1 89 6c 24 10 e8 ?? ?? ?? ?? 85 c0 74 09",
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
	HookGuard hookGuard("hitDetectionMain");
	endScene.onHitDetectionStart(hitDetectionType);
	if (hitDetectionType == 1 && !gifMode.modDisabled) {
		throws.hitDetectionMainHook();
	}
	{
		std::unique_lock<std::mutex> guard(throws.orig_hitDetectionMainMutex);
		throws.orig_hitDetectionMain(this, hitDetectionType);
	}
	if (!gifMode.modDisabled) {
		endScene.onHitDetectionEnd(hitDetectionType);
	}
}
#else
BOOL Throws::HookHelp::hitDetectionMainHook(char* other) {
	HookGuard hookGuard("hitDetectionMain");
	throws.hitDetectionMainHook();
	BOOL result;
	{
		std::unique_lock<std::mutex> guard(throws.orig_hitDetectionMainMutex);
		result throws.orig_hitDetectionMain((char*)this, other);
	}
	return result;
}
#endif

void Throws::hitDetectionMainHook() {
	entityList.populate();
	for (int i = 0; i < entityList.count; ++i) {
		Entity ent = entityList.list[i];

		const AttackType attackType = ent.attackType();
		const bool isActive = ent.isActive();
		int throwRange = ent.throwRange();  // This value resets to -1 on normal throws very fast so that's why we need this separate hook.
		                                    // For command throws it stays non -1 for much longer than the throw actually happens

		const int throwMinX = ent.throwMinX();
		const int throwMaxX = ent.throwMaxX();

		const int throwMaxY = ent.throwMaxY();
		const int throwMinY = ent.throwMinY();

		const unsigned int currentAnimDuration = ent.currentAnimDuration();
		CharacterType charType = ent.characterType();
		CharacterType ownerType = (CharacterType)(-1);
		if (charType == -1) {
			ownerType = entityList.slots[ent.team()].characterType();
		}

		bool checkPassed = (throwRange >= 0 || throwMinX < throwMaxX || throwMinY < throwMaxY)
			&& (attackType == ATTACK_TYPE_NORMAL  // ATTACK_TYPE_NORMAL is more like 4/6 H (+OS possibly) throw
			|| (attackType == ATTACK_TYPE_EX  // ATTACK_TYPE_EX is a command throw
			|| attackType == ATTACK_TYPE_OVERDRIVE  // ATTACK_TYPE_OVERDRIVE is Jack-O super throw
			|| attackType == ATTACK_TYPE_IK)  // ATTACK_TYPE_IK is Potemkin's IK - it's a throw
			&& isActive
			&& !(currentAnimDuration > 25 && charType == CHARACTER_TYPE_AXL) // the 25 check is needed to stop Axl from showing a throwbox all throughout his Yes move
		);

		bool isMettagiri = charType == CHARACTER_TYPE_FAUST
			&& strcmp(ent.animationName(), "Mettagiri") == 0
			&& currentAnimDuration <= 1;
		if (isMettagiri) {
			throwRange = 175000;
			checkPassed = true;
		}

		if (checkPassed) {

			const int posX = ent.posX();
			const int flip = ent.isFacingLeft() ? -1 : 1;

			ThrowInfo throwInfo;
			throwInfo.attackType = attackType;
			throwInfo.active = true;
			throwInfo.owner = ent;
			throwInfo.isPawn = ent.isPawn();
			throwInfo.framesLeft = DISPLAY_DURATION_THROW;
			throwInfo.isMettagiri = isMettagiri;

			if (throwRange >= 0) {
				throwInfo.hasPushboxCheck = true;
				int otherLeft;
				int otherRight;

				Entity other = nullptr;
				if (entityList.count >= 2) {
					other = entityList.slots[1 - ent.team()];
				} else {
					other = entityList.slots[0];
				}
				other.pushboxLeftRight(&otherLeft, &otherRight);
				int otherX = other.posX();

				throwInfo.leftUnlimited = false;
				throwInfo.rightUnlimited = false;
				ent.pushboxLeftRight(&throwInfo.left, &throwInfo.right);
				throwInfo.pushboxCheckMinX = throwInfo.left - throwRange;
				throwInfo.pushboxCheckMaxX = throwInfo.right + throwRange;
				throwInfo.left -= throwRange + (otherRight - otherX);
				throwInfo.right += throwRange + (otherX - otherLeft);
			}

			if (throwMinX < throwMaxX) {
				throwInfo.hasXCheck = true;
				int throwMinXInSpace = posX + throwMinX * flip;
				int throwMaxXInSpace = posX + throwMaxX * flip;
				
				if (throwMinXInSpace > throwMaxXInSpace) {
					std::swap(throwMinXInSpace, throwMaxXInSpace);
				}
				
				if (throwInfo.hasPushboxCheck
						&& throwMinXInSpace > throwInfo.pushboxCheckMinX
						&& throwMaxXInSpace < throwInfo.pushboxCheckMaxX) {
					throwInfo.hasPushboxCheck = false;
				}

				throwInfo.minX = throwMinXInSpace;
				throwInfo.maxX = throwMaxXInSpace;

				if (throwInfo.leftUnlimited || throwMinXInSpace > throwInfo.left) throwInfo.left = throwMinXInSpace;
				if (throwInfo.rightUnlimited || throwMaxXInSpace < throwInfo.right) throwInfo.right = throwMaxXInSpace;
				throwInfo.leftUnlimited = false;
				throwInfo.rightUnlimited = false;
			}

			if (throwMinY < throwMaxY) {
				throwInfo.hasYCheck = true;
				const int posY = ent.posY();
				const int throwMinYInSpace = posY + throwMinY;
				const int throwMaxYInSpace = posY + throwMaxY;
				throwInfo.minY = throwMinYInSpace;
				throwInfo.maxY = throwMaxYInSpace;

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
	unsigned int currentTime = *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
	if (previousTime != currentTime) {
		previousTime = currentTime;
		timeHasChanged = true;
	}

	auto it = infos.begin();
	while (it != infos.end()) {
		ThrowInfo& throwInfo = *it;
		
		if (timeHasChanged && !throwInfo.firstFrame) {
			throwInfo.active = false;
			--throwInfo.framesLeft;
			if (throwInfo.framesLeft <= 0) {
				it = infos.erase(it);
				continue;
			}
		}
		throwInfo.firstFrame = false;
		
		// This is to prevent 4H/6H from displaying 1 active frame at the start and startup 1.
		if (throwInfo.active
				&& (throwInfo.attackType != ATTACK_TYPE_NONE
				&& throwInfo.attackType != ATTACK_TYPE_NORMAL
				|| endScene.didHit(throwInfo.owner)
				|| !throwInfo.isPawn)
				&& !throwInfo.isMettagiri) {
			for (int i = 0; i < 2; ++i) {
				if (throwInfo.owner == entityList.slots[i]) {
					PlayerInfo& player = endScene.players[i];
					++player.hitboxesCount;
					break;
				}
			}
		}
		
		if (!invisChipp.needToHide(throwInfo.owner)) {
			if (!settings.drawPushboxCheckSeparately) {
	
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
	
				graphics.drawDataPrepared.throwBoxes.push_back(params);
	
			} else {
				
				if (throwInfo.hasPushboxCheck) {
	
					DrawBoxCallParams params;
					params.fillColor = replaceAlpha(throwInfo.active ? 64 : 0, COLOR_THROW_PUSHBOX);
					params.outlineColor = replaceAlpha(255, COLOR_THROW_PUSHBOX);
					params.thickness = THICKNESS_THROW_PUSHBOX;
					params.left = throwInfo.pushboxCheckMinX;
					params.right = throwInfo.pushboxCheckMaxX;
					params.bottom = -10000000;
					params.top = 10000000;
					graphics.drawDataPrepared.throwBoxes.push_back(params);
	
				}
	
				if (throwInfo.hasXCheck || throwInfo.hasYCheck) {
	
					DrawBoxCallParams params;
					params.fillColor = replaceAlpha(throwInfo.active ? 64 : 0, COLOR_THROW_XYORIGIN);
					params.outlineColor = replaceAlpha(255, COLOR_THROW_XYORIGIN);
					params.thickness = THICKNESS_THROW_XYORIGIN;
	
					params.left = -10000000;
					params.right = 10000000;
					params.bottom = -10000000;
					params.top = 10000000;
	
					if (throwInfo.hasXCheck) {
						params.left = throwInfo.minX;
						params.right = throwInfo.maxX;
					}
	
					if (throwInfo.hasYCheck) {
						params.top = throwInfo.maxY;
						params.bottom = throwInfo.minY;
					}
	
					graphics.drawDataPrepared.throwBoxes.push_back(params);
	
				}
	
			}
		}
		++it;
	}
}

void Throws::clearAllBoxes() {
	infos.clear();
}
