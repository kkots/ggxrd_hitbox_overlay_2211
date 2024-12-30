#pragma once
#include <atomic>

class GifMode
{
public:
	std::atomic_bool gifModeOn{ false };
	std::atomic_bool noGravityOn{ false };
	std::atomic_bool modDisabled{ false };
	std::atomic_bool gifModeToggleBackgroundOnly{ false };
	std::atomic_bool gifModeToggleCameraCenterOnly{ false };
	std::atomic_bool toggleCameraCenterOpponent{ false };
	std::atomic_bool gifModeToggleHideOpponentOnly{ false };
	std::atomic_bool toggleHidePlayer{ false };
	std::atomic_bool gifModeToggleHudOnly{ false };
	std::atomic_bool showInputHistory{ true };
};

extern GifMode gifMode;
