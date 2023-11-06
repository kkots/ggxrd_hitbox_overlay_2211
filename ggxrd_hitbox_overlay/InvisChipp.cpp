#include "pch.h"
#include "InvisChipp.h"
#include "Game.h"
#include "EntityList.h"
#include "characterTypes.h"
#include "logging.h"

InvisChipp invisChipp;

void InvisChipp::onEndSceneStart() {
	p1IsInvisChipp = false;
	p2IsInvisChipp = false;
	if (game.currentModeIsOnline() && entityList.count >= 2) {
		p1IsInvisChipp = determineInvisChipp(entityList.slots[0]);
		p2IsInvisChipp = determineInvisChipp(entityList.slots[1]);
	}
}

bool InvisChipp::determineInvisChipp(const char* aswData) {
	return (Entity(aswData).characterType() == CHARACTER_TYPE_CHIPP)
		&& *(short*)(aswData + 0x24C50) != 0;
}

bool InvisChipp::isP1InvisChipp() const {
	return p1IsInvisChipp;
}

bool InvisChipp::isP2InvisChipp() const {
	return p2IsInvisChipp;
}

bool InvisChipp::needToHide(const Entity& ent) const {
	return needToHide(ent.team());
}

bool loggedPlayerSideOnce = false;

bool InvisChipp::needToHide(char team) const {
	char playerSide = game.getPlayerSide();
	if (!loggedPlayerSideOnce) {
		loggedPlayerSideOnce = true;
		logwrap(fprintf(logfile, "You're on player %hhd side\n", playerSide));
	}
	if (playerSide == 2) return false;
	if (team == 0) {
		if (playerSide == 1) {
			return isP1InvisChipp();
		} else {
			return false;
		}
	} else {
		if (playerSide == 0) {
			return isP2InvisChipp();
		} else {
			return false;
		}
	}
}
