#pragma once
#include <d3d9.h>
#include "DrawHitboxArrayParams.h"
#include "Hitbox.h"

RECT combineBounds(const RECT& a, const RECT& b);

void combineBounds(RECT& result, const RECT& other);

struct DrawHitboxArrayCallParams {
	const Hitbox* hitboxData = nullptr;
	int hitboxCount = 0;
	DrawHitboxArrayParams params{ 0 };
	D3DCOLOR fillColor{ 0 };
	D3DCOLOR outlineColor{ 0 };
	int thickness = 1;
	bool hatched = false;
	int originX = 0;
	int originY = 0;
	bool operator==(const DrawHitboxArrayCallParams& other) const;
	bool operator!=(const DrawHitboxArrayCallParams& other) const;
	RECT getWorldBounds(const Hitbox& hitbox, int cos = -2000, int sin = -2000) const;
	inline RECT getWorldBounds(int hitboxIndex, int cos = -2000, int sin = -2000) const { return getWorldBounds(hitboxData[hitboxIndex], cos, sin); }
	RECT getWorldBounds() const;
};
