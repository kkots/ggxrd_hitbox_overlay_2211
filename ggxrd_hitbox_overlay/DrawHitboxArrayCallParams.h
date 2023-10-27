#pragma once
#include <d3d9.h>
#include "DrawHitboxArrayParams.h"

struct Hitbox;  // I've had enough cross-reference errors in my life

struct DrawHitboxArrayCallParams {
	const Hitbox* hitboxData = nullptr;
	int hitboxCount = 0;
	DrawHitboxArrayParams params{ 0 };
	D3DCOLOR fillColor{ 0 };
	D3DCOLOR outlineColor{ 0 };
	bool operator==(const DrawHitboxArrayCallParams& other) const;
	bool operator!=(const DrawHitboxArrayCallParams& other) const;
};
