#include "pch.h"
#include "HitDetector.h"
#include "collectHitboxes.h"
#include "memoryFunctions.h"
#include "Detouring.h"
#include "EntityList.h"
#include "InvisChipp.h"
#include "Graphics.h"
#include "logging.h"

HitDetector hitDetector;

const int hitboxThatHitDisplayTime = 5; // in frames

bool HitDetector::onDllMain() {
	bool error = false;

	orig_hitDetection = (hitDetection_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x74\x12\x85\xed\x75\x0e\x85\xdb\x74\x39\x85\xff\x74\x35\x8b\x74\x24\x14\xeb\x1d\x83\xfd\x01",
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
	}
	return hitResult;
}

void HitDetector::drawDetected() {
	auto it = hitboxesThatHit.begin();
	while (it != hitboxesThatHit.end()) {
		DetectedHitboxes& hitboxThatHit = *it;

		bool entityInTheList = false;
		for (auto i = 0; i < entityList.count; i++) {
			Entity ent { entityList.list[i] };
			// this is needed for Sol's Gunflame. The gunflame continues to exist as entity but stops being active as soon as it hits
			if (hitboxThatHit.attacker == ent) {
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
			bool needToHide = false;
			if (hitboxThatHit.attackerTeam == 0) {
				needToHide = invisChipp.isP1InvisChipp();
			}
			else if (entityList.count > 1) {
				needToHide = invisChipp.isP2InvisChipp();
			}
			if (needToHide) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
			graphics.hitboxes.push_back(hitboxThatHit.hitboxes);
			--hitboxThatHit.counter;
			if (hitboxThatHit.counter <= 0) {
				it = hitboxesThatHit.erase(it);
				continue;
			}
		}
		++it;
	}
}
