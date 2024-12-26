#pragma once
#include <vector>
#include "ThrowInfo.h"

using hitDetectionMain_t = void (__thiscall*)(void* aswSubengine, int hitDetectionType);


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
		void hitDetectionMainHook(int hitDetectionType);
	};

	hitDetectionMain_t orig_hitDetectionMain = nullptr;

	void hitDetectionMainHook();

	std::vector<ThrowInfo> infos;

	unsigned int previousTime = 0;
};

extern Throws throws;
