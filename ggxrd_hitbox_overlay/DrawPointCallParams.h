#pragma once
#include <d3d9.h>

struct DrawPointCallParams {
	int posX = 0;
	int posY = 0;
	D3DCOLOR fillColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	D3DCOLOR outlineColor = D3DCOLOR_ARGB(255, 0, 0, 0);
};
