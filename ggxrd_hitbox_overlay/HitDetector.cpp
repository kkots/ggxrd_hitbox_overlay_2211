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

HitDetector hitDetector;

const int hitboxThatHitDisplayTime = 5; // in frames
const int hurtboxThatGotHitDisplayTime = 10; // in frames

bool HitDetector::onDllMain() {
	bool error = false;

	char orig_hitDetectionSig[] = "\x74\x12\x85\xed\x75\x0e\x85\xdb\x74\x39\x85\xff\x74\x35\x8b\x74\x24\x14\xeb\x1d\x83\xfd\x01";
	orig_hitDetection = (hitDetection_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		orig_hitDetectionSig,
		_countof(orig_hitDetectionSig),
		{-0x3D},
		&error, "hitDetection");

	BOOL(HookHelp::*hookPtr)(void*, int, int, int*, int*) = &HookHelp::hitDetectionHook;
	if (!detouring.attach(&(PVOID&)(orig_hitDetection),
		(PVOID&)hookPtr,
		"hitDetection")) return false;

	return !error;
}

void HitDetector::clearAllBoxes() {
	hitboxesThatHit.clear();
}

BOOL HitDetector::HookHelp::hitDetectionHook(void* defender, int attackerHitboxIndex, int defenderHitboxIndex, int* intersectionX, int* intersectionY) {
	// this  ==  attacker
	BOOL hitResult = hitDetector.orig_hitDetection(this, defender, attackerHitboxIndex, defenderHitboxIndex, intersectionX, intersectionY);
	if (hitResult) {
		std::vector<DrawHitboxArrayCallParams> theHitbox;
		Entity thisEntity{ (char*)this };
		collectHitboxes(thisEntity, true, nullptr, &theHitbox, nullptr, nullptr);
		if (!theHitbox.empty()) {
			hitDetector.hitboxesThatHit.push_back({ thisEntity, thisEntity.team(), theHitbox.front(), hitboxThatHitDisplayTime });
		}

		DrawHitboxArrayCallParams theHurtbox;
		Entity otherEntity{ (char*)defender };
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

			EntityState state;
			otherEntity.getState(&state);

			hitDetector.hurtboxesThatGotHit.push_back({
				otherEntity, otherEntity.team(), theHurtbox, hurtboxThatGotHitDisplayTime, state.counterhit});
		}
	}
	return hitResult;
}

HitDetector::WasHitInfo HitDetector::wasThisHitPreviously(Entity ent) const {
	for (auto it = hurtboxesThatGotHit.cbegin(); it != hurtboxesThatGotHit.cend(); ++it) {
		if (ent == it->entity) {
			return {true, it->hitboxes, it->counterhit};
		}
	}
	return {false, DrawHitboxArrayCallParams{}, false};
}

void HitDetector::drawHits() {
	bool timeHasChanged = false;
	unsigned int currentTime = entityList.getCurrentTime();
	if (previousTime != currentTime) {
		previousTime = currentTime;
		timeHasChanged = true;
	}

	if (timeHasChanged) {
		auto it = hurtboxesThatGotHit.begin();
		while (it != hurtboxesThatGotHit.end()) {
			--it->counter;
			if (it->counter == 0) {
				hurtboxesThatGotHit.erase(it);
				continue;
			}
			++it;
		}
	}

	auto it = hitboxesThatHit.begin();
	while (it != hitboxesThatHit.end()) {
		DetectedHitboxes& hitboxThatHit = *it;

		bool entityInTheList = false;
		for (auto i = 0; i < entityList.count; i++) {
			Entity ent { entityList.list[i] };
			// this is needed for Sol's Gunflame. The gunflame continues to exist as entity but stops being active as soon as it hits
			if (hitboxThatHit.entity == ent) {
				const bool entityIsActive = (*(unsigned int*)(ent + 0x23C) & 0x100) != 0;
				if (entityIsActive) {
					entityInTheList = true;
				}
				break;
			}
		}

		if (entityInTheList) {
			it = hitboxesThatHit.erase(it);
			continue;
		}
		else {
			if (invisChipp.isTeamsInvisChipp(hitboxThatHit.team)) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
			graphics.hitboxes.push_back(hitboxThatHit.hitboxes);
			if (timeHasChanged) {
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
