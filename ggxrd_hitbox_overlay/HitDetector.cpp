#include "pch.h"
#include "HitDetector.h"
#include "collectHitboxes.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "EntityList.h"
#include "InvisChipp.h"
#include "Graphics.h"
#include "logging.h"
#include "colors.h"
#include "GifMode.h"
#include "Game.h"

HitDetector hitDetector;

const int rejectionDistance = 300000;
const int REJECTION_DELAY = 0;
const int hitboxMinActiveTime = 2;

bool HitDetector::onDllMain() {
	bool error = false;

	// ghidra sig: 55 8b ec 83 e4 f8 81 ec 54 02 00 00 a1 ?? ?? ?? ?? 33 c4 89 84 24 50 02 00 00 8b 45 10 53 56 8b 75 08 8b 5e 10 f7 db 57 8b f9 1b db 23 de f6 87 64 04 00 00 02
	orig_determineHitType = (determineHitType_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x55\x8b\xec\x83\xe4\xf8\x81\xec\x54\x02\x00\x00\xa1\x00\x00\x00\x00\x33\xc4\x89\x84\x24\x50\x02\x00\x00\x8b\x45\x10\x53\x56\x8b\x75\x08\x8b\x5e\x10\xf7\xdb\x57\x8b\xf9\x1b\xdb\x23\xde\xf6\x87\x64\x04\x00\x00\x02",
		"xxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
		nullptr, "determineHitType");

	if (orig_determineHitType) {
		int(HookHelp::*determineHitTypeHookPtr)(void*, BOOL, unsigned int*, unsigned int*) = &HookHelp::determineHitTypeHook;
		detouring.attach(&(PVOID&)(orig_determineHitType),
			(PVOID&)determineHitTypeHookPtr,
			&orig_determineHitTypeMutex,
			"determineHitType");
	}

	return !error;
}

void HitDetector::clearAllBoxes() {
	std::unique_lock<std::mutex> guard(mutex);
	hitboxesThatHit.clear();
	hurtboxesThatGotHit.clear();
	rejections.clear();
}

/* 0 means no hit
   1 is get hit (including by all throws)
   2 is blocked hit
   3 is ignoring hit due to playing a possibly very long throw animation (dunno why that is separate)
   4 is rejected hit */
int HitDetector::HookHelp::determineHitTypeHook(void* defender, BOOL wasItType10Hitbox, unsigned int* param3, unsigned int* hpPtr) {
	class HookTracker {
	public:
		HookTracker() {
			++detouring.hooksCounter;
			detouring.markHookRunning("determineHitType", true);
		}
		~HookTracker() {
			detouring.markHookRunning("determineHitType", false);
			--detouring.hooksCounter;
		}
	} hookTracker;
	int result;
	{
		std::unique_lock<std::mutex> guard(hitDetector.orig_determineHitTypeMutex);
		result = hitDetector.orig_determineHitType(this, defender, wasItType10Hitbox, param3, hpPtr);
	}
	if (gifMode.modDisabled) return result;
	if (!gifMode.hitboxDisplayDisabled) {
		std::unique_lock<std::mutex> guard(hitDetector.mutex);
		Entity thisEntity{ (char*)this };
		Entity otherEntity{ (char*)defender };
		if (result == 4) {
			if (thisEntity.characterType() != -1
				|| !otherEntity
				|| (*(unsigned int*)(otherEntity + 0x10) == 0)) return result;

			// "CounterGuard..."  +  "..Stand", "..Air", "..Crouch"
			if (strncmp(otherEntity.animationName(), "CounterGuard", 12) == 0) {
				for (auto it = hitDetector.rejections.begin(); it != hitDetector.rejections.end(); ++it) {
					if (it->owner == otherEntity) {
						hitDetector.rejections.erase(it);
						break;
					}
				}

				int posY = otherEntity.posY();

				Rejection newRejection;
				newRejection.owner = otherEntity;
				newRejection.firstFrame = true;
				newRejection.counter = DISPLAY_DURATION_REJECTION + REJECTION_DELAY;
				newRejection.skipFrame = -1;
				newRejection.activeFrame = DISPLAY_DURATION_REJECTION;
				newRejection.left = otherEntity.posX() - rejectionDistance;
				newRejection.right = newRejection.left + rejectionDistance * 2;
				newRejection.top = posY + rejectionDistance;
				newRejection.bottom = posY - rejectionDistance;
				hitDetector.rejections.push_back(newRejection);
			}
		}

		if ((result == 1 || result == 2)
				&& (DISPLAY_DURATION_HITBOX_THAT_HIT || DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT)) {
			EntityState state;
			otherEntity.getState(&state);
			if (!state.strikeInvuln) {

				if (DISPLAY_DURATION_HITBOX_THAT_HIT && thisEntity.characterType() == -1) {
					std::vector<DrawHitboxArrayCallParams> theHitbox;
					collectHitboxes(thisEntity, true, nullptr, &theHitbox, nullptr, nullptr);
					if (!theHitbox.empty()) {
						DetectedHitboxes boxes;
						boxes.entity = thisEntity;
						boxes.team = thisEntity.team();
						boxes.hitboxes = theHitbox.front();
						boxes.counter = DISPLAY_DURATION_HITBOX_THAT_HIT;
						boxes.previousTime = thisEntity.currentAnimDuration();

						hitDetector.hitboxesThatHit.push_back(boxes);
					}
				}

				if (DISPLAY_DURATION_HURTBOX_THAT_GOT_HIT) {
					DrawHitboxArrayCallParams theHurtbox;
					collectHitboxes(otherEntity, true, &theHurtbox, nullptr, nullptr, nullptr);
					DWORD oldTransparency = theHurtbox.fillColor >> 24;
					theHurtbox.fillColor = replaceAlpha(oldTransparency == 0 ? 64 : oldTransparency, COLOR_HURTBOX_OLD);
					theHurtbox.outlineColor = replaceAlpha(255, COLOR_HURTBOX_OLD);
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
	}

	if ((gifMode.gifModeOn || gifMode.gifModeToggleHideOpponentOnly) && game.isTrainingMode()) {
		result = 0;
	}
	return result;
}

HitDetector::WasHitInfo HitDetector::wasThisHitPreviously(Entity ent, const DrawHitboxArrayCallParams& currentHurtbox) {
	std::unique_lock<std::mutex> guard(mutex);
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
	std::unique_lock<std::mutex> guard(mutex);
	bool timeHasChanged = false;
	unsigned int currentTime = entityList.getCurrentTime();
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

			graphics.drawDataPrepared.throwBoxes.push_back(params);
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
				Entity ent { entityList.list[i] };
				// this is needed for Sol's Gunflame. The gunflame continues to exist as entity but stops being active as soon as it hits
				// this is also needed for Chipp's Gamma Blade. It stops being active very soon after it hits
				if (hitboxThatHit.entity == ent) {
					entityInTheList = true;
					const bool isActive = (*(unsigned int*)(ent + 0x23C) & 0x100) != 0;
					const int hitboxCount = *(const int*)(ent + 0xA4);
					entityInTheListAndActive = isActive && hitboxCount > 0;
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
			if (it->activeTime >= hitboxMinActiveTime) {
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
			graphics.drawDataPrepared.hitboxes.push_back(hitboxThatHit.hitboxes);

			if (hitboxThatHit.timeHasChanged(timeHasChanged)) {
				--hitboxThatHit.counter;
				if (hitboxThatHit.counter <= 0) {
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
