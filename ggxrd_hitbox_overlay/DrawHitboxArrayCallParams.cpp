#include "pch.h"
#include "DrawHitboxArrayCallParams.h"
#include "Hitbox.h"
#include <utility>
#include "pi.h"
#include <cmath>

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

RECT DrawHitboxArrayCallParams::getWorldBounds(int index, int cos, int sin) const {
	RECT result;
	
	int offX = params.scaleX * ((int)hitboxData[index].offX + params.hitboxOffsetX / 1000 * params.flip);
	int offY = params.scaleY * (-(int)hitboxData[index].offY + params.hitboxOffsetY / 1000);
	int sizeX = (int)hitboxData[index].sizeX * params.scaleX;
	int sizeY = -(int)hitboxData[index].sizeY * params.scaleY;
	
	if (params.angle) {
		int centerX = offX + sizeX / 2;
		int centerY = offY + sizeY / 2;
		
		int angleCapped = params.angle % 360000;
		if (angleCapped < 0) angleCapped += 360000;
		
		if (cos == -2000 || sin == -2000) {
			float angleRads = -(float)params.angle / 1000.F / 180.F * PI;
			cos = (int)(::cos(angleRads) * 1000.F);
			sin = (int)(::sin(angleRads) * 1000.F);
		}
		
		if (angleCapped >= 45000 && (angleCapped < 135000 || angleCapped >= 225000)) {
			std::swap(sizeX, sizeY);
		}
		offX = (cos * centerX - sin * centerY) / 1000 - sizeX / 2;
		offY = (cos * centerY + sin * centerX) / 1000 - sizeY / 2;
	}

	offX -= params.hitboxOffsetX;
	offX = params.posX + offX * params.flip;
	sizeX *= params.flip;
	offY += params.posY + params.hitboxOffsetY;
	
	result.left = offX;
	result.right = offX + sizeX;
	result.top = offY;
	result.bottom = offY + sizeY;
	
	if (result.left > result.right) {
		std::swap(result.left, result.right);
	}
	if (result.top > result.bottom) {
		std::swap(result.top, result.bottom);
	}
	
	return result;
}

RECT DrawHitboxArrayCallParams::getWorldBounds() const {
	RECT result;
	if (!hitboxCount) {
		memset(&result, 0, sizeof RECT);
		return result;
	}
	
	RECT subresult;
	for (int i = 0; i < hitboxCount; ++i) {
		subresult = getWorldBounds(i);
		if (i == 0) {
			result = subresult;
			continue;
		}
		combineBounds(result, subresult);
	}
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
