#pragma once
#include <d3d9.h>

struct DrawBoxCallParams {
	int left = 0;
	int top = 0;
	int right = 0;
	int bottom = 0;
	D3DCOLOR fillColor{ 0 };
	D3DCOLOR outlineColor{ 0 };
	int thickness = 0;
	int originX = 0;
	int originY = 0;
	bool hatched = false;
};
