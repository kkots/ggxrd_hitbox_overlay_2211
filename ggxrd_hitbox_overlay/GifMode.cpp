#include "pch.h"
#include "GifMode.h"

GifMode gifMode;

void GifMode::updateFPS() {
	fpsApplied = modDisabled ? 60.F : fpsSetting * fpsSpeedUpReplay / 60.F;
}
