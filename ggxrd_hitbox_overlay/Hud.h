#pragma once
#include <mutex>

using REDHUDBattleUpdateAll_t = void(__thiscall*)(char* thisArg);

class Hud
{
public:
	bool onDllMain();
	void onDllDetach();
private:
	class HookHelp {
		friend class Hud;
		void REDHUDBattleUpdateAllHook();
	};
	void REDHUDBattleUpdateAllHook(char* thisArg);
	void changeHudVisibility(bool isVisible);
	char* hudPtr = nullptr;
	REDHUDBattleUpdateAll_t orig_REDHUDBattleUpdateAll;
	std::mutex orig_REDHUDBattleUpdateAllMutex;
};

extern Hud hud;
