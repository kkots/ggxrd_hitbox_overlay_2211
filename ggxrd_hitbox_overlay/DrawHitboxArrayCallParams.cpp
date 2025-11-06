#include "pch.h"
#include "Hitbox.h"
#include "DrawHitboxArrayCallParams.h"
#include <utility>
#include "pi.h"
#include <cmath>
#include "Graphics.h"

bool DrawHitboxArrayCallParams::operator==(const DrawHitboxArrayCallParams& other) const {
	if (!(data.size() == other.data.size()
		&& params.flip == other.params.flip
		&& params.scaleX == other.params.scaleX
		&& params.scaleY == other.params.scaleY
		&& params.angle == other.params.angle
		&& (params.posX - other.params.posX < 1000 && params.posX - other.params.posX > -1000)
		&& (params.posY - other.params.posY < 1000 && params.posY - other.params.posY > -1000))) return false;
	
	const Hitbox* hitboxPtr = data.data();
	const Hitbox* hitboxOtherPtr = other.data.data();
	if (hitboxPtr == hitboxOtherPtr) return true;

	for (int counter = (int)data.size(); counter != 0; --counter) {
		if (*hitboxPtr != *hitboxOtherPtr) {
			return false;
		}

		++hitboxPtr;
		++hitboxOtherPtr;
	}
	return true;
}

bool DrawHitboxArrayCallParams::operator!=(const DrawHitboxArrayCallParams& other) const {
	return !(*this == other);
}

// this function ('s coordinate math) has been reversed almost straight from the game
RECT DrawHitboxArrayCallParams::getWorldBounds(const Hitbox& hitbox, int cos, int sin) const {
	
	// The raw Hitbox data has been composed for the sprites facing left while X is pointed to the right,
	// Y axis pointing down and all distances divided by 1000,
	// in comparison to the arena coordinate system, where an unmirrored sprite faces right, with X still pointing to the right,
	// Y axis pointing up, and distances are 1000 times larger.
	
	// The following calculations take place in the modified hitbox designer coordinate space, which is the same as the
	// raw Hitbox data coordinate space, but with the exception that the Y axis is pointing up.
	// The angle is inverted here because it is specified for the sprite facing right, while the hitbox was designed for a sprite facing left.
	int angle = -params.angle % 360000;
	if (angle < 0) angle = angle + 360000;
	
	int x = (int)hitbox.offX;
	int y = -(int)hitbox.offY;  // inverted, because Y axis is pointing up instead of down
	int sizeX = params.scaleX * (int)hitbox.sizeX;
	int sizeY = params.scaleY * -(int)hitbox.sizeY;
	
	// get point relative to transform center
	x -= params.flip * (params.transformCenterX / 1000);
	y -= params.transformCenterY / 1000;
	
	x *= params.scaleX;  // 1000 scale (the default) means 1.0
	y *= params.scaleY;
	
	if (angle != 0) {
		int centerX = sizeX / 2 + x;
		int centerY = sizeY / 2 + y;
		if (cos == -2000 || sin == -2000) {
			int angleDiv100 = angle / 100;
			cos = Graphics::getCos(angleDiv100);
			sin = Graphics::getSin(angleDiv100);
		}
		if (angle >= 45000 && (angle < 135000 || angle >= 225000)) {  // (sic)
			std::swap(sizeX, sizeY);
		}
		x = (cos * centerX - sin * centerY) / 1000 - sizeX / 2;
		y = (cos * centerY + sin * centerX) / 1000 - sizeY / 2;
	}
	
	// translate point from transform center relative to global
	x += params.flip * params.transformCenterX;
	y += params.transformCenterY;
	
	RECT result;
	result.left = params.posX + params.flip * x;
	result.right = params.posX + params.flip * (x + sizeX);
	result.top = params.posY + y;
	result.bottom = params.posY + y + sizeY;
	if (result.right < result.left) {
		std::swap(result.right, result.left);
	}
	if (result.bottom < result.top) {
		std::swap(result.bottom, result.top);
	}
	return result;
}

RECT DrawHitboxArrayCallParams::getWorldBounds() const {
	RECT result;
	if (data.empty()) {
		memset(&result, 0, sizeof RECT);
		return result;
	}
	
	int angle = -params.angle % 360000;
	if (angle < 0) angle = angle + 360000;
	int cos = -2000;
	int sin = -2000;
	if (angle) {
		int angleDiv100 = angle / 100;
		cos = Graphics::getCos(angleDiv100);
		sin = Graphics::getSin(angleDiv100);
	}
	
	RECT subresult;
	int hitboxCount = (int)data.size();
	for (int i = 0; i < hitboxCount; ++i) {
		subresult = getWorldBounds(data[i], cos, sin);
		if (i == 0) {
			result = subresult;
			continue;
		}
		combineBounds(result, subresult);
	}
	#pragma warning(suppress: 6001)  // ignore warning about uninitialized memory usage
	return result;
}

RECT combineBounds(const RECT& a, const RECT& b) {
	return {
		min(a.left, b.left),
		min(a.top, b.top),
		max(a.right, b.right),
		max(a.bottom, b.bottom)
	};
}

void combineBounds(RECT& result, const RECT& other) {
	if (other.left < result.left) result.left = other.left;
	if (other.right > result.right) result.right = other.right;
	if (other.top < result.top) result.top = other.top;
	if (other.bottom > result.bottom) result.bottom = other.bottom;
}

void DrawHitboxArrayCallParams::arenaToHitbox(const DrawHitboxArrayParams& params, int left, int top, int right, int bottom, Hitbox* outData) {
	
	if (left > right) std::swap(left, right);
	if (top > bottom) std::swap(top, bottom);
	
	// need to unrotate the hitbox. The params.angle is taken with a negative sign when transforming JONBIN coordinates to
	// world coordinates, and we invert that here
	int angleReverse = params.angle % 360000;
	if (angleReverse < 0) angleReverse = angleReverse + 360000;
	
	int angle = -params.angle % 360000;
	if (angle < 0) angle = angle + 360000;
	
	left -= params.posX;
	right -= params.posX;
	top -= params.posY;
	bottom -= params.posY;
	
	if (params.flip == -1) {
		int tmp = left;
		left = -right;
		right = -tmp;
	}
	
	int sizeX = right - left;
	int sizeY = bottom - top;
	
	left -= params.flip * params.transformCenterX;
	top -= params.transformCenterY;
	
	if (angle != 0) {
		int centerX = sizeX / 2 + left;
		int centerY = sizeY / 2 + top;
		int angleDiv100 = angleReverse / 100;
		int cos = Graphics::getCos(angleDiv100);
		int sin = Graphics::getSin(angleDiv100);
		if (angle >= 45000 && (angle < 135000 || angle >= 225000)) {  // (sic)
			std::swap(sizeX, sizeY);
		}
		left = (cos * centerX - sin * centerY) / 1000 - sizeX / 2;
		top = (cos * centerY + sin * centerX) / 1000 - sizeY / 2;
	}
	
	if (params.scaleX == 0 || params.scaleY == 0) {
		outData->type = 0;
		outData->offX = 0.F;
		outData->offY = 0.F;
		outData->sizeX = 0.F;
		outData->sizeY = 0.F;
		return;
	}
	
	float leftFloat = (float)left / (float)params.scaleX;
	float topFloat = (float)top / (float)params.scaleY;
	
	// get point relative to transform center
	leftFloat += (float)(params.flip * (params.transformCenterX / 1000));
	topFloat += (float)(params.transformCenterY / 1000);
	
	outData->type = 0;
	outData->offX = std::floorf(leftFloat);
	outData->offY = std::floorf(-topFloat);
	outData->sizeX = std::floorf((float)sizeX / (float)params.scaleX);
	outData->sizeY = std::floorf((float)-sizeY / (float)params.scaleY);
	
	if (outData->sizeY < 0.F) {
		outData->offY += outData->sizeY;
		outData->sizeY = -outData->sizeY;
	}
	
	if (outData->sizeX < 0.F) {
		outData->offX += outData->sizeX;
		outData->sizeX = -outData->sizeX;
	}
	
}

bool DrawHitboxArrayCallParams::willFlipWidthAndHeight(int angle) {
	angle = -angle % 360000;
	if (angle < 0) angle = angle + 360000;
	
	if (angle != 0) {
		return angle >= 45000 && (angle < 135000 || angle >= 225000);  // (sic)
	}
	return false;
}
