#include "pch.h"
#include "EntityList.h"
#include "Game.h"
#include "logging.h"

EntityList entityList;

void EntityList::populate() {

	count = *(int*)(*aswEngine + 0xB4);
	list = (Entity*)(*aswEngine + 0x1FC);
	slots = (Entity*)(*aswEngine + 0xC8);

}

unsigned int EntityList::getCurrentTime() {
	if (slots && count >= 0) {
		unsigned int p1Time = slots[0].currentAnimDuration();
		if (count >= 2) {
			unsigned int p2Time = slots[1].currentAnimDuration();
			if (previousTime != p2Time) {
				previousTime = p2Time;
				return p2Time;
			}
		}
		if (previousTime != p1Time) {
			previousTime = p1Time;
			return p1Time;
		}
		return previousTime;
	}
	return 0;
}

bool EntityList::areAnimationsNormal() const {
	if (slots && count >= 1) {
		if (!isAnimationNormal(slots[0])) return false;
	}
	if (slots && count >= 2) {
		if (!isAnimationNormal(slots[1])) return false;
	}
	return true;
}

bool EntityList::isAnimationNormal(Entity ent) const {
	if (strncmp(ent.animationName(), "Event", 5) == 0) return false;
	// Ichigeki is instant kill but the animation is that even before the IK lands so we can't use it
	if (ent.cmnActIndex() == CmnActResultLose) return false;
	if (ent.cmnActIndex() == CmnActResultWin) return false;
	if (strcmp(ent.animationName(), "entry_default") == 0) return false;  // can there be non-default entry?..
	if (ent.cmnActIndex() == CmnActEntryWait) return false;  // completely invisible
	if (ent.cmnActIndex() == CmnActMatchWin) return false;  // victory animation, is skipped on IK
	return true;
}
