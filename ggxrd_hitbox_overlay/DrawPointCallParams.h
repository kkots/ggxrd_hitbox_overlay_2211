#pragma once
#include <d3d9.h>

struct DrawPointCallParams {
	bool isProjectile = false;
	int posX = 0;
	int posY = 0;
	D3DCOLOR fillColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	D3DCOLOR outlineColor = D3DCOLOR_ARGB(255, 0, 0, 0);
};

struct DrawLineCallParams {
	int posX1 = 0;
	int posY1 = 0;
	int posX2 = 0;
	int posY2 = 0;
	D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 255);
};

struct DrawCircleCallParams {
	int posX = 0;
	int posY = 0;
	int radius = 0;
	D3DCOLOR fillColor = D3DCOLOR_ARGB(0, 0, 0, 0);
	D3DCOLOR outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
};
