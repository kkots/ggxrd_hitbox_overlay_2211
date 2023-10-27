#include "pch.h"
#include "DrawHitboxArrayCallParams.h"
#include "Hitbox.h"

bool DrawHitboxArrayCallParams::operator==(const DrawHitboxArrayCallParams& other) const {
	if (!(hitboxCount == other.hitboxCount
		&& params.flip == other.params.flip
		&& params.scaleX == other.params.scaleX
		&& params.scaleY == other.params.scaleY
		&& params.angle == other.params.angle
		&& params.posX == other.params.posX
		&& params.posY == other.params.posY
		&& fillColor == other.fillColor)) return false;
	
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
