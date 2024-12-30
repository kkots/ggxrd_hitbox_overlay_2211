#pragma once
#include "InputsDrawingCommand.h"

const int INPUTS_ICON_SIZE = 41;
const float INPUTS_ICON_ATLAS_WIDTH = 1536.F;
const float INPUTS_ICON_ATLAS_HEIGHT = 1536.F;

struct InputsIcon {
	float uStart;
	float vStart;
	float uEnd;
	float vEnd;
	InputsIcon() = default;
	InputsIcon(float uStart, float vStart, float uEnd, float vEnd)
		: uStart(uStart), vStart(vStart), uEnd(uEnd), vEnd(vEnd) { }
	inline constexpr InputsIcon(int x, int y)
		: uStart((float)x / 1536.F),
		vStart((float)y / 1536.F),
		uEnd((float)(x + INPUTS_ICON_SIZE) / INPUTS_ICON_ATLAS_WIDTH),
		vEnd((float)(y + INPUTS_ICON_SIZE) / INPUTS_ICON_ATLAS_HEIGHT) {
	}
};

extern const InputsIcon inputsIcon[INPUT_ICON_LAST];
