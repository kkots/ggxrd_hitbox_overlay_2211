#pragma once
#include "ComplicatedHurtbox.h"
#include "DrawPointCallParams.h"
#include "DrawBoxCallParams.h"
#include <vector>

struct DrawData {
	std::vector<ComplicatedHurtbox> hurtboxes;
	std::vector<DrawHitboxArrayCallParams> hitboxes;
	std::vector<DrawBoxCallParams> pushboxes;
	std::vector<DrawPointCallParams> points;
	std::vector<DrawBoxCallParams> throwBoxes;
	void clear();
	void copyTo(DrawData* destination);
	bool needTakeScreenshot = false;
};

struct CameraValues {
	D3DXVECTOR3 pos{ 0.F, 0.F, 0.F };
	float forward[3]{ 0.F };
	float right[3]{ 0.F };
	float up[3]{ 0.F };
	float fov = 0.F;
	float coordCoefficient = 0.F;
	bool setValues();
	void copyTo(CameraValues& destination);
};
