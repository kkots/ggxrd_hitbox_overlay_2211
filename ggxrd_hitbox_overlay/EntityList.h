#pragma once
#include "Entity.h"

class EntityList
{
public:
	void populate();
	int count = 0;
	Entity* list = nullptr;
	Entity* slots = nullptr;
	unsigned int getCurrentTime();
	bool areAnimationsNormal() const;
	bool isAnimationNormal(Entity ent) const;
private:
	unsigned int previousTime = 0;
};

extern EntityList entityList;
