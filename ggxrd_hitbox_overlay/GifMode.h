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
	std::atomic_bool dontHideOpponentsEffects{ false };
	std::atomic_bool dontHideOpponentsBoxes{ false };
	std::atomic_bool toggleHidePlayer{ false };
	std::atomic_bool dontHidePlayersEffects{ false };
	std::atomic_bool dontHidePlayersBoxes{ false };
	std::atomic_bool gifModeToggleHudOnly{ false };
	std::atomic_bool showInputHistory{ true };
	std::atomic_bool allowCreateParticles{ true };
};

extern GifMode gifMode;
