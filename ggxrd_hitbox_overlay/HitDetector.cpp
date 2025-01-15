#include "pch.h"
#include "HitDetector.h"
#include "collectHitboxes.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "EntityList.h"
#include "Entity.h"
#include "InvisChipp.h"
#include "logging.h"
#include "colors.h"
#include "GifMode.h"
#include "Game.h"
#include "EndScene.h"

HitDetector hitDetector;

const int rejectionDistance = 300000;
const int REJECTION_DELAY = 0;
const int hitboxMinActiveTime = 2;

bool HitDetector::onDllMain() {
	bool error = false;
	
	uintptr_t activeFrameHitCallPlace = sigscanOffset(
		"GuiltyGearXrd.exe",
		"e8 ?? ?? ?? ?? 8b 87 84 09 00 00 83 f8 04 74 13 83 f8 05 74 0e 83 f8 01 74 09 83 f8 02 0f 85 89 fe ff ff",
		&error, "activeFrameHit");
	if (activeFrameHitCallPlace) {
		activeFrameHit = followRelativeCall(activeFrameHitCallPlace);
	}
	
	uintptr_t determineHitTypeCallPlace = 0;
	if (activeFrameHit) {
		determineHitTypeCallPlace = sigscanForward(activeFrameHit,
			"e8 ?? ?? ?? ?? f7 87 a4 09 00 00 00 40 00 00",
			0x100);
	}
	if (determineHitTypeCallPlace) {
		orig_determineHitType = (determineHitType_t)followRelativeCall(determineHitTypeCallPlace);
	}
	if (!orig_determineHitType) {
		logwrap(fprintf(logfile, "determineHitType not found\n"));
	} else {
		HitResult(HookHelp::*determineHitTypeHookPtr)(void*, BOOL, DWORD*, int*) = &HookHelp::determineHitTypeHook;
		detouring.attach(&(PVOID&)(orig_determineHitType),
			(PVOID&)determineHitTypeHookPtr,
			"determineHitType");
	}
	
	uintptr_t copyDealtAtkToReceivedAtkCallPlace = 0;
	if (activeFrameHit) {
		copyDealtAtkToReceivedAtkCallPlace = sigscanForward(activeFrameHit,
			"e8 ?? ?? ?? ?? 53 55 8b cf e8 ?? ?? ?? ?? 83 fd 01 74 0f 83 fd 02 74 0a 83 fd 04 74 05 83 fd 05 75 14",
			0x2000);
	}
	
	if (copyDealtAtkToReceivedAtkCallPlace) {
		orig_copyDealtAtkToReceivedAtk = (copyDealtAtkToReceivedAtk_t)followRelativeCall(copyDealtAtkToReceivedAtkCallPlace);
	}
	
	if (!orig_copyDealtAtkToReceivedAtk) {
		logwrap(fprintf(logfile, "copyDealtAtkToReceivedAtk not found\n"));
	} else {
		void(HookHelp::*copyDealtAtkToReceivedAtkHookPtr)(void*) = &HookHelp::copyDealtAtkToReceivedAtkHook;
		detouring.attach(&(PVOID&)(orig_copyDealtAtkToReceivedAtk),
			(PVOID&)copyDealtAtkToReceivedAtkHookPtr,
			"copyDealtAtkToReceivedAtk");
	}
	
	uintptr_t dealHitCallPlace = 0;
	if (activeFrameHit) {
		dealHitCallPlace = sigscanForward(activeFrameHit,
			"83 be dc 07 00 00 02 75 0a c7 86 b4 9f 00 00 01 00 00 00 8b 54 24 34 52 53 8b ce e8 ?? ?? ?? ??",
			0x2000);
	}
	if (dealHitCallPlace) {
		orig_dealHit = (dealHit_t)followRelativeCall(dealHitCallPlace + 0x1b);
	}
	
	if (!orig_dealHit) {
		logwrap(fprintf(logfile, "dealHit not found\n"));
	} else {
		void(HookHelp::*dealHitHookPtr)(void*, BOOL) = &HookHelp::dealHitHook;
		detouring.attach(&(PVOID&)(orig_dealHit),
			(PVOID&)dealHitHookPtr,
			"dealHit");
	}
	
	return !error;
}

void HitDetector::clearAllBoxes() {
	hitboxesThatHit.clear();
	hurtboxesThatGotHit.clear();
	rejections.clear();
}

HitResult HitDetector::HookHelp::determineHitTypeHook(void* defender, BOOL wasItType10Hitbox, DWORD* hitFlags, int* hpPtr) {
	HitResult result = hitDetector.orig_determineHitType(this, defender, wasItType10Hitbox, hitFlags, hpPtr);
	if (gifMode.modDisabled) return result;
	Entity thisEntity{ (char*)this };
	Entity otherEntity{ (char*)defender };
	if (result == HIT_RESULT_ARMORED) {
		if (thisEntity.characterType() == -1
				&& otherEntity
				&& otherEntity.isPawn()) {
			
			// "CounterGuard..."  +  "..Stand", "..Air", "..Crouch"
			if (strncmp(otherEntity.animationName(), "CounterGuard", 12) == 0) {
				for (auto it = hitDetector.rejections.begin(); it != hitDetector.rejections.end(); ++it) {
					if (it->owner == otherEntity) {
						hitDetector.rejections.erase(it);
						break;
					}
				}
	
				int posY = otherEntity.posY();
				int posX = otherEntity.posX();
	
				Rejection newRejection;
				newRejection.owner = otherEntity;
				newRejection.firstFrame = true;
				newRejection.counter = DISPLAY_DURATION_REJECTION + REJECTION_DELAY;
				newRejection.skipFrame = -1;
				newRejection.activeFrame = DISPLAY_DURATION_REJECTION;
				newRejection.left = posX - rejectionDistance;
				newRejection.right = newRejection.left + rejectionDistance * 2;
				newRejection.top = posY + rejectionDistance;
				newRejection.bottom = posY - rejectionDistance;
				newRejection.hatched = false;
				newRejection.originX = posX;
				newRejection.originY = posY;
				hitDetector.rejections.push_back(newRejection);
			}
		}
	}

	int hitboxesCount = 0;
	std::vector<DrawHitboxArrayCallParams> theHitbox;
	collectHitboxes(thisEntity,
		true,
		nullptr,
		&theHitbox,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		&hitboxesCount);
	
	
	bool hasHitbox = !theHitbox.empty();
	
	if ((result == HIT_RESULT_NORMAL || result == HIT_RESULT_BLOCKED || result == HIT_RESULT_ARMORED)
			&& (DISPLAY_DURATION_HITBOX_THAT_HIT || DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT)) {
		EntityState state;
		otherEntity.getState(&state);
		if (!state.strikeInvuln) {

			if (DISPLAY_DURATION_HITBOX_THAT_HIT && hasHitbox) {
				DetectedHitboxes boxes;
				boxes.entity = thisEntity;
				boxes.isPawn = thisEntity.isPawn();
				boxes.team = thisEntity.team();
				boxes.hitboxes = theHitbox.front();
				boxes.counter = DISPLAY_DURATION_HITBOX_THAT_HIT;
				boxes.previousTime = thisEntity.currentAnimDuration();
				boxes.hitboxesCount = hitboxesCount;
				hitDetector.hitboxesThatHit.push_back(boxes);
			}

			if (DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT) {
				DrawHitboxArrayCallParams theHurtbox;
				collectHitboxes(otherEntity,
					true,
					&theHurtbox,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr);
				
				DWORD oldTransparency = theHurtbox.fillColor >> 24;
				theHurtbox.fillColor = replaceAlpha(oldTransparency == 0 ? 64 : oldTransparency, COLOR_HURTBOX_OLD);
				theHurtbox.outlineColor = replaceAlpha(255, COLOR_HURTBOX_OLD);
				theHurtbox.hatched = false;
				if (theHurtbox.hitboxCount) {
					for (auto it = hitDetector.hurtboxesThatGotHit.begin(); it != hitDetector.hurtboxesThatGotHit.end(); ++it) {
						if (it->entity == otherEntity) {
							hitDetector.hurtboxesThatGotHit.erase(it);
							break;
						}
					}

					DetectedHitboxes boxes;
					boxes.entity = otherEntity;
					boxes.team = otherEntity.team();
					boxes.hitboxes = theHurtbox;
					boxes.counter = DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT;
					boxes.previousTime = otherEntity.currentAnimDuration();

					hitDetector.hurtboxesThatGotHit.push_back(boxes);
				}
			}
		}
	}
	
	if (result == HIT_RESULT_NORMAL
			|| result == HIT_RESULT_BLOCKED
			|| result == HIT_RESULT_ARMORED
			|| result == HIT_RESULT_ARMORED_BUT_NO_DMG_REDUCTION
			|| result == HIT_RESULT_IGNORED && (*hitFlags & 0x40) != 0) {  // flag 0x40 set when it needs to destroy the projectile that got hit
		endScene.registerHit(result, hasHitbox, thisEntity, otherEntity);
	}

	if (endScene.isEntityHidden(otherEntity) && game.isTrainingMode()) {
		result = HIT_RESULT_NONE;
	}
	return result;
}

void HitDetector::HookHelp::copyDealtAtkToReceivedAtkHook(void* attacker) {
	hitDetector.orig_copyDealtAtkToReceivedAtk((void*)this, attacker);
	endScene.onAfterAttackCopy(Entity{(char*)this}, Entity{attacker});
}

void HitDetector::HookHelp::dealHitHook(void* attacker, BOOL isInHitstun) {
	endScene.onDealHit(Entity{(char*)this}, Entity{attacker});
	hitDetector.orig_dealHit((void*)this, attacker, isInHitstun);
	endScene.onAfterDealHit(Entity{(char*)this}, Entity{attacker});
}

HitDetector::WasHitInfo HitDetector::wasThisHitPreviously(Entity ent, const DrawHitboxArrayCallParams& currentHurtbox) {
	for (auto it = hurtboxesThatGotHit.cbegin(); it != hurtboxesThatGotHit.cend(); ++it) {
		if (ent == it->entity) {
			if (currentHurtbox == it->hitboxes && it->counter < DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT) {
				hurtboxesThatGotHit.erase(it);
				return { false, DrawHitboxArrayCallParams{} };
			}
			return {true, it->hitboxes};
		}
	}
	return {false, DrawHitboxArrayCallParams{}};
}

void HitDetector::drawHits() {
	bool timeHasChanged = false;
	unsigned int currentTime = *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
	if (previousTime != currentTime) {
		previousTime = currentTime;
		timeHasChanged = true;
	}

	auto rejIt = rejections.begin();
	while (rejIt != rejections.end()) {
		Rejection& rejection = *rejIt;
		if (invisChipp.needToHide(rejection.owner)) {
			rejIt = rejections.erase(rejIt);
			continue;
		}

		if (timeHasChanged && !rejection.firstFrame) {
			--rejection.counter;
			if (rejection.counter <= 0) {
				rejIt = rejections.erase(rejIt);
				continue;
			}
		}
		rejection.firstFrame = false;

		if (rejection.counter != rejection.skipFrame) {
			DrawBoxCallParams params;
			params.fillColor = replaceAlpha(rejection.counter == rejection.activeFrame ? 64 : 0, COLOR_REJECTION);
			params.outlineColor = replaceAlpha(255, COLOR_REJECTION);
			params.thickness = THICKNESS_REJECTION;

			params.left = rejection.left;
			params.right = rejection.right;
			params.bottom = rejection.bottom;
			params.top = rejection.top;
			
			params.hatched = rejection.hatched;
			params.originX = rejection.originX;
			params.originY = rejection.originY;

			endScene.drawDataPrepared.throwBoxes.push_back(params);
		}
		++rejIt;
	}

	{
		auto it = hurtboxesThatGotHit.begin();
		while (it != hurtboxesThatGotHit.end()) {
			if ((*it).timeHasChanged(timeHasChanged)) {
				--it->counter;
				if (it->counter == 0) {
					it = hurtboxesThatGotHit.erase(it);
					continue;
				}
			}
			++it;
		}
	}

	auto it = hitboxesThatHit.begin();
	while (it != hitboxesThatHit.end()) {
		DetectedHitboxes& hitboxThatHit = *it;

		bool entityInTheList = false;
		bool entityInTheListAndActive = false;
		if (hitboxThatHit.entity) {
			for (auto i = 0; i < entityList.count; i++) {
				Entity ent = entityList.list[i];
				// this is needed for Sol's Gunflame. The gunflame continues to exist as entity but stops being active as soon as it hits
				// this is also needed for Chipp's Gamma Blade. It stops being active very soon after it hits
				if (ent.isActive() && hitboxThatHit.entity == ent) {
					entityInTheList = true;
					const bool isActive = (*(unsigned int*)(ent + 0x23C) & 0x100) != 0;
					const int hitboxCount = *(const int*)(ent + 0xA4);
					entityInTheListAndActive = ent.isActiveFrames() && hitboxCount > 0;
					break;
				}
			}
		}

		if (!entityInTheList) {
			hitboxThatHit.entity = nullptr;
		}

		if (entityInTheListAndActive) {
			if (timeHasChanged) {
				++it->activeTime;
			}
			if (it->activeTime >= hitboxMinActiveTime || it->isPawn) {
				it = hitboxesThatHit.erase(it);
			} else {
				++it;
			}
			continue;
		}
		else {
			if (invisChipp.needToHide(hitboxThatHit.team)) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
			endScene.drawDataPrepared.hitboxes.push_back(hitboxThatHit.hitboxes);

			if (hitboxThatHit.timeHasChanged(timeHasChanged)) {
				--hitboxThatHit.counter;
				if (hitboxThatHit.counter <= 0 || hitboxThatHit.isPawn) {
					it = hitboxesThatHit.erase(it);
					continue;
				}
			}
		}
		++it;
	}
}

bool HitDetector::DetectedHitboxes::timeHasChanged(bool globalTimeHasChanged) {
	if (entity) {
		const unsigned int newTime = entity.currentAnimDuration();
		if (newTime != previousTime) {
			previousTime = newTime;
			return true;
		}
		return false;
	} else {
		return globalTimeHasChanged;
	}
}
