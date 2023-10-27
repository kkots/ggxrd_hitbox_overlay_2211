#pragma once

// We get hitboxes from the game so we're not allowed to modify anything
struct Hitbox
{
	const int type;
	const float offX;
	const float offY;
	const float sizeX;
	const float sizeY;
	bool operator==(const Hitbox& other) const;
	bool operator!=(const Hitbox& other) const;
};
