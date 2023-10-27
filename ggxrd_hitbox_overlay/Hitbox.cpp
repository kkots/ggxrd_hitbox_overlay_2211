#include "pch.h"
#include "Hitbox.h"

bool Hitbox::operator==(const Hitbox& other) const {
	return type == other.type
		&& offX == other.offX
		&& offY == other.offY
		&& sizeX == other.sizeX
		&& sizeY == other.sizeY;
}

bool Hitbox::operator!=(const Hitbox& other) const {
	return !(*this == other);
}
