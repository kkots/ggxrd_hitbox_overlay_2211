#pragma once
#include "DrawHitboxArrayCallParams.h"

struct ComplicatedHurtbox {
	bool hasTwo = false;
	DrawHitboxArrayCallParams realbox;
	DrawHitboxArrayCallParams graybox;
};
