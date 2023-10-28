#pragma once
#include <d3d9.h>

namespace {
	D3DCOLOR COLOR_PUSHBOX = D3DCOLOR_ARGB(0, 255, 255, 0);
	D3DCOLOR COLOR_HITBOX = D3DCOLOR_ARGB(0, 255, 0, 0);
	D3DCOLOR COLOR_HURTBOX = D3DCOLOR_ARGB(0, 0, 255, 0);
	D3DCOLOR COLOR_HURTBOX_COUNTERHIT = D3DCOLOR_ARGB(0, 0, 255, 255);
	D3DCOLOR COLOR_HURTBOX_OLD = D3DCOLOR_ARGB(0, 128, 128, 128);
	D3DCOLOR COLOR_THROW = D3DCOLOR_ARGB(0, 0, 0, 255);

	D3DCOLOR replaceAlpha(DWORD alpha, D3DCOLOR color) {
		return (alpha << 24) | (color & 0xFFFFFF);
	}
}