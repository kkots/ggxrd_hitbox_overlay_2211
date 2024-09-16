#pragma once
class AltModes
{
public:
	bool onDllMain();
	bool isGameInNormalMode(bool* needToClearHitDetection, bool* isPauseMenu = nullptr);
	char roundendCameraFlybyType() const;
private:
	char* roundendCameraFlybyTypeRef = nullptr;
	char* versusModeMenuOpenRef = nullptr;
	char* isIKCutscenePlaying = nullptr;
	char** pauseMenu = nullptr;
};

extern AltModes altModes;
