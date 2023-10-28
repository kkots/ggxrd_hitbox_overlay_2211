#pragma once
#include "Entity.h"

struct ThrowInfo {
	Entity owner{ nullptr };
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
