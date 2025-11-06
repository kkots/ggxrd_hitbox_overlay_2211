#pragma once
#include "DrawHitboxArrayCallParams.h"

struct ComplicatedHurtbox {
	bool hasTwo = false;
	DrawHitboxArrayCallParams realbox;
	DrawHitboxArrayCallParams graybox;
	inline ComplicatedHurtbox() { }
	inline ComplicatedHurtbox(const DrawHitboxArrayCallParams& hurtbox) : hasTwo(false), realbox(hurtbox) { }
	inline ComplicatedHurtbox(const DrawHitboxArrayCallParams& hurtbox, const DrawHitboxArrayCallParams& graybox)
		: hasTwo(true), realbox(hurtbox), graybox(graybox) { }
};
