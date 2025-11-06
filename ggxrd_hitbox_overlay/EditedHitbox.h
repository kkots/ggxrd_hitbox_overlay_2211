#pragma once
#include "Hitbox.h"
#include "Entity.h"
struct EditedHitbox {
	HitboxType type;
	bool isPushbox = false;
	int originalIndex = 0;
	int subindex = 0;  // might deviate from actual hitboxIndex when using layers. Is out of bounds by 1 when isPushbox and not using layers
	Hitbox* ptr = nullptr;
};
