#pragma once
#include "characterTypes.h"

using getPos_t = int(__thiscall*)(const void*);
using getPushbox_t = int(__thiscall*)(const void*);

class Entity
{
public:
	Entity(const char* ent);
	const char* ent = nullptr;

	// active means the attack frames are coming
	bool isActive() const;

	// 0 for player 1's team, 1 for player 2's team
	char team() const;

	CharacterType characterType() const;

	int posX() const;
	int posY() const;

	bool isFacingLeft() const;

	bool isDoingAThrow() const;

	bool isGettingThrown() const;

	int pushboxWidth() const;
	int pushboxTop() const;
	int pushboxFrontWidthOffset() const;
	int pushboxBottom() const;

	char* operator+(int offset) const;

	bool operator==(const Entity& other) const;

};

class EntityManager {
public:
	bool onDllMain();
private:
	friend class Entity;
	getPos_t getPosX;
	getPos_t getPosY;
	getPushbox_t getPushboxWidth;
	getPushbox_t getPushboxTop;
	getPushbox_t getPushboxBottom;
};

extern EntityManager entityManager;