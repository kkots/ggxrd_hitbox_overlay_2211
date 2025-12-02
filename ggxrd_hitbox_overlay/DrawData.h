#pragma once
#include "ComplicatedHurtbox.h"
#include "DrawPointCallParams.h"
#include "DrawBoxCallParams.h"
#include <vector>
#include "InputsDrawingCommand.h"

enum GameModeFast {
	GAME_MODE_FAST_NORMAL,
	GAME_MODE_FAST_TUTORIAL,
	GAME_MODE_FAST_CHALLENGE,
	GAME_MODE_FAST_KENTEI
};

struct DrawData {
	std::vector<ComplicatedHurtbox> hurtboxes;
	std::vector<DrawHitboxArrayCallParams> hitboxes;
	std::vector<DrawBoxCallParams> pushboxes;
	std::vector<DrawBoxCallParams> interactionBoxes;
	std::vector<DrawPointCallParams> points;
	std::vector<DrawCircleCallParams> circles;
	std::vector<DrawLineCallParams> lines;
	std::vector<DrawBoxCallParams> throwBoxes;
	std::vector<InputsDrawingCommandRow> inputs[2] { std::vector<InputsDrawingCommandRow>{}, std::vector<InputsDrawingCommandRow>{} };
	bool inputsContainsDurations = false;
	size_t inputsSize[2] { 0 };
	GameModeFast gameModeFast;
	inline void clear() { clearBoxes(); clearInputs(); }
	void clearBoxes();
	void clearInputs();
	void copyTo(DrawData* destination);
	bool needTakeScreenshot = false;
};

struct CameraValues {
	D3DXVECTOR3 pos{ 0.F, 0.F, 0.F };
	int pitch = 0;
	int yaw = 0;
	int roll = 0;
	float forward[3]{ 0.F };
	float right[3]{ 0.F };
	float up[3]{ 0.F };
	float fov = 0.F;
	float coordCoefficient = 0.F;
	bool setValues();
	void copyTo(CameraValues& destination);
};
