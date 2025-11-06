#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "DrawData.h"

using updateDarken_t = void(__thiscall*)(char* thisArg);
using updateCamera_t = void(__thiscall*)(char* thisArg, char** param1, char* param2);

struct CameraPos {
	float x = 0.F;
	float y = 0.F;
	float z = 0.F;
	int pitch = 0;
	int yaw = 0;
	int roll = 0;
	float fov = 0.F;
};

class Camera
{
public:
	void onEndSceneStart();
	bool worldToScreen(IDirect3DDevice9* device, const D3DXVECTOR3& vec, D3DXVECTOR3* out);
	bool onDllMain();
	void updateDarkenHook(char* thisArg);
	void updateCameraHook(char* thisArg, char** param1, char* param2);
	void grabValues();
	void updateCameraManually();
	CameraValues valuesPrepare;
	CameraValues valuesUse;
	bool grabbedValues = false;
	bool shutdown = false;
	float editHitboxesOffsetX = 0.F;
	float editHitboxesOffsetY = 0.F;
	float editHitboxesViewDistance = 0.F;
	bool frozen = false;
	bool lastGameTickRanFine = true;
private:
	friend struct CameraValues;
	class HookHelp {
		friend class Camera;
		void updateDarkenHook();
		void updateCameraHook(char** param1, char* param2);
	};
	updateDarken_t orig_updateDarken = nullptr;
	updateCamera_t orig_updateCamera = nullptr;
	unsigned int darkenValue1Offset = 0;
	unsigned int cameraOffset = 0;
	bool isSet = false;
	float clipXHalf = 0.F;
	float clipYHalf = 0.F;
	float multiplier = 0.F;
	void setValues(IDirect3DDevice9* device);
	float convertCoord(float in) const;
	void angleVectors(
		const float p, const float y, const float r,
		float* forward, float* right, float* up) const;
	float vecDot(float* a, float* b) const;
	uintptr_t coordCoeffOffset = 0;
	void updateCameraPos(CameraPos* camPos);
};

extern Camera camera;
