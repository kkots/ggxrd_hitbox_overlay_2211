#pragma once

struct Hitbox
{
	int type;
	float offX;
	float offY;
	float sizeX;
	float sizeY;
	bool operator==(const Hitbox& other) const;
	inline bool operator!=(const Hitbox& other) const { return !(*this == other); }
};
