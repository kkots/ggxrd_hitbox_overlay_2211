#pragma once

struct DrawHitboxArrayParams {
	int posX = 0;
	int posY = 0;
	char flip = 1;  // 1 for facing left, -1 for facing right
	int scaleX = 1000;
	int scaleY = 1000;
	int angle = 0;
	int hitboxOffsetX = 0;
	int hitboxOffsetY = 0;
};
