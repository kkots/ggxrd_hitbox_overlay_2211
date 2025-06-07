#include "pch.h"
#include "Hitbox.h"
#include "DrawHitboxArrayCallParams.h"
#include <utility>
#include "pi.h"
#include <cmath>
#include "Graphics.h"

bool DrawHitboxArrayCallParams::operator==(const DrawHitboxArrayCallParams& other) const {
	if (!(hitboxCount == other.hitboxCount
		&& params.flip == other.params.flip
		&& params.scaleX == other.params.scaleX
		&& params.scaleY == other.params.scaleY
		&& params.angle == other.params.angle
		&& (params.posX - other.params.posX < 1000 && params.posX - other.params.posX > -1000)
		&& (params.posY - other.params.posY < 1000 && params.posY - other.params.posY > -1000))) return false;
	
	const Hitbox* hitboxPtr = hitboxData;
	const Hitbox* hitboxOtherPtr = other.hitboxData;
	if (hitboxPtr == hitboxOtherPtr) return true;

	for (int counter = hitboxCount; counter != 0; --counter) {
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
	
	// The raw Hitbox data has been composed for the sprites facing left, Y axis pointing down and all distances divided by 1000,
	// in comparison to the arena coordinate system.
	
	// The following calculations take place in the modified hitbox designer coordinate space, which is the same as the
	// raw Hitbox data coordinate space, but with the exception that the Y axis is pointing up.
	// The angle is inverted here because it is specified for the sprite facing right.
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
	x += params.flip * params.transformCenterX;  // scaleX is not included here, because (sic)
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
	if (!hitboxCount) {
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
	for (int i = 0; i < hitboxCount; ++i) {
		subresult = getWorldBounds(hitboxData[i], cos, sin);
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
