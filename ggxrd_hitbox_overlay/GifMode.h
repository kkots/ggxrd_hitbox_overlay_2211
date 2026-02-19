#pragma once

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
	bool editHitboxes { false };
	void* editHitboxesEntity { nullptr };
	float fpsSetting { 60.F };
	float fpsSpeedUpReplay { 60.F };
	float fpsApplied { 60.F };
	bool speedUpReplay { false };
	void updateFPS();
	bool mostModDisabled { false };
};

extern GifMode gifMode;
