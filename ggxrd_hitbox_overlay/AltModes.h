#pragma once
class AltModes
{
public:
	bool onDllMain();
	bool isGameInNormalMode(bool* needToClearHitDetection);
private:
	char* trainingModeMenuOpenRef = nullptr;
	char* versusModeMenuOpenRef = nullptr;
	char* isIKCutscenePlaying = nullptr;
};

extern AltModes altModes;
