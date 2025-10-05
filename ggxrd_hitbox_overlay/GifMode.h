#pragma once
#include <array>

class GifMode
{
public:
	bool gifModeOn{ false };
	bool noGravityOn{ false };
	bool modDisabled{ false };
	bool gifModeToggleBackgroundOnly{ false };
	bool gifModeToggleCameraCenterOnly{ false };
	bool toggleCameraCenterOpponent{ false };
	bool gifModeToggleHideOpponentOnly{ false };
	bool dontHideOpponentsEffects{ false };
	bool dontHideOpponentsBoxes{ false };
	bool toggleHidePlayer{ false };
	bool dontHidePlayersEffects{ false };
	bool dontHidePlayersBoxes{ false };
	bool gifModeToggleHudOnly{ false };
	bool showInputHistory{ true };
	bool allowCreateParticles{ true };
	bool makeOpponentFullInvul { false };
	bool makePlayerFullInvul { false };
};

extern GifMode gifMode;
