#include "pch.h"
#include "GifMode.h"

GifMode gifMode;

void GifMode::updateFPS() {
	fpsApplied = fpsSetting * fpsSpeedUpReplay / 60.F;
}
