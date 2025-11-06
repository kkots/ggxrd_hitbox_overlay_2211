#pragma once

struct DrawHitboxArrayParams {
	int posX = 0;
	int posY = 0;
	char flip = 1;  // 1 for facing left, -1 for facing right
	int scaleX = 1000;
	int scaleY = 1000;
	int angle = 0;  // 1000 means one degree counter-clockwise, if facing right. Gets mirrored with facing, so when facing left, it's clockwise instead
	int transformCenterX = 0;  // does not get mirrored with sprite facing, i.e. if positive and facing right, it transforms around the front, but if positive and facing left, it transforms around the back
	int transformCenterY = 0;
};
