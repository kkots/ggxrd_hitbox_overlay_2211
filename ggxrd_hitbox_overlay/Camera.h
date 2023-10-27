#pragma once
#include <d3d9.h>
#include <d3dx9.h>

class Camera
{
public:
	void onEndSceneStart();
	void worldToScreen(IDirect3DDevice9* device, const D3DXVECTOR3& vec, D3DXVECTOR3* out);
private:
	bool isSet = false;
	float forward[3]{ 0.F };
	float right[3]{ 0.F };
	float up[3]{ 0.F };
	D3DXVECTOR3 pos{ 0.F, 0.F, 0.F };
	float clipX = 0.F;
	float clipY = 0.F;
	float clipXHalf = 0.F;
	float clipYHalf = 0.F;
	float divisor = 0.F;
	void setValues(IDirect3DDevice9* device);
	float convertCoord(float in) const;
	void angleVectors(
		const float p, const float y, const float r,
		float* forward, float* right, float* up) const;
	float vecDot(float* a, float* b) const;
};

extern Camera camera;