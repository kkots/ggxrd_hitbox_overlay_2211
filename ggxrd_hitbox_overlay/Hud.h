#pragma once
#include "MutexWhichTellsWhatThreadItsLockedBy.h"

using updateHud_t = void(__thiscall*)(char* thisArg);

class Hud
{
public:
	bool onDllMain();
	void onDllDetach();
private:
	class HookHelp {
		friend class Hud;
		void updateHudHook();
	};
	void updateHudHook(char* thisArg);
	void changeHudVisibility(bool isVisible);
	char* hudPtr = nullptr;
	updateHud_t orig_updateHud;
	MutexWhichTellsWhatThreadItsLockedBy orig_updateHudMutex;
};

extern Hud hud;
