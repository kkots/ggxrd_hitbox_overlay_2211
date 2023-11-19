#pragma once
#include <atomic>

class GifMode
{
public:
	std::atomic_bool gifModeOn{ false };
	std::atomic_bool noGravityOn{ false };
	std::atomic_bool modDisabled{ false };
	std::atomic_bool hitboxDisplayDisabled{ false };
	std::atomic_bool gifModeToggleBackgroundOnly{ false };
	std::atomic_bool gifModeToggleCameraCenterOnly{ false };
	std::atomic_bool gifModeToggleHideOpponentOnly{ false };
};

extern GifMode gifMode;
