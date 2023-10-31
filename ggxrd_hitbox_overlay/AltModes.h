#pragma once
class AltModes
{
public:
	bool onDllMain();
	bool isGameInNormalMode(bool* needToClearHitDetection);
	char roundendCameraFlybyType() const;
private:
	char* trainingModeMenuOpenRef = nullptr;
	char* roundendCameraFlybyTypeRef = nullptr;
	char* versusModeMenuOpenRef = nullptr;
	char* isIKCutscenePlaying = nullptr;
};

extern AltModes altModes;
