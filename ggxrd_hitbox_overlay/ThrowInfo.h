#pragma once
#include "Entity.h"

struct ThrowInfo {
	Entity owner{ nullptr };

	bool hasPushboxCheck = false;
	int pushboxCheckMinX = 0;
	int pushboxCheckMaxX = 0;

	bool hasXCheck = false;
	int minX = 0;
	int maxX = 0;

	bool hasYCheck = false;
	int minY = 0;
	int maxY = 0;

	bool leftUnlimited = true;
	int left = 0;
	bool rightUnlimited = true;
	int right = 0;
	bool topUnlimited = true;
	int top = 0;
	bool bottomUnlimited = true;
	int bottom = 0;
	bool active = true;
	int framesLeft = 0;
	bool firstFrame = true;
};
