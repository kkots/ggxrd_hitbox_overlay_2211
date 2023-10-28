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
	GameMode gameMode = game.getGameMode();
	if (!(gameMode == GAME_MODE_ARCADE
		|| gameMode == GAME_MODE_CHALLENGE
		|| gameMode == GAME_MODE_REPLAY
		|| gameMode == GAME_MODE_STORY
		|| gameMode == GAME_MODE_TRAINING
		|| gameMode == GAME_MODE_TUTORIAL
		|| gameMode == GAME_MODE_VERSUS)
		&& entityList.count >= 2) {

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

bool InvisChipp::isCorrespondingChippInvis(const Entity& ent) const {
	return isTeamsInvisChipp(ent.team());
}

bool InvisChipp::isTeamsInvisChipp(char team) const {
	if (team == 0) {
		return isP1InvisChipp();
	} else {
		return isP2InvisChipp();
	}
}
