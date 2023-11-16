#pragma once
#include <atomic>

class GifMode
{
public:
	std::atomic_bool gifModeOn{ false };
	std::atomic_bool noGravityOn{ false };
	std::atomic_bool modDisabled{ false };
};

extern GifMode gifMode;
