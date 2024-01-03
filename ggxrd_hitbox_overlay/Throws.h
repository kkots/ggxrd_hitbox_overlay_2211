#pragma once
#include <vector>
#include "ThrowInfo.h"
#include <mutex>

#ifndef USE_ANOTHER_HOOK
using hitDetectionMain_t = void (__thiscall*)(void* aswSubengine, int hitDetectionType);
#else
using hitDetectionMain_t = BOOL(__thiscall*)(char* thisPtr, char* other);
#endif

class Throws
{
public:
	bool onDllMain();
	void drawThrows();
	void clearAllBoxes();
private:
	class HookHelp {
	private:
		friend class Throws;
#ifndef USE_ANOTHER_HOOK
		void hitDetectionMainHook(int hitDetectionType);
#else
		BOOL hitDetectionMainHook(char* other);
#endif
	};

	hitDetectionMain_t orig_hitDetectionMain = nullptr;
	std::mutex orig_hitDetectionMainMutex;

	void hitDetectionMainHook();

	std::vector<ThrowInfo> infos;

	unsigned int previousTime = 0;
};

extern Throws throws;
