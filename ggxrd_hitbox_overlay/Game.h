#pragma once
#include "gameModes.h"
#include <mutex>

extern const char** aswEngine;

using gameLoop_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using actualGameLoop_t = void(__thiscall*)(char* thisArg, int param1);
using trainingHudTick_t = void(__thiscall*)(char* thisArg);
using getTrainingHudArgument_t = char*(__cdecl*)(void);
using updateAnimations_t = void(__cdecl*)(int param1, int param2, int param3, int param4);

class Game {
public:
	bool onDllMain();
	GameMode getGameMode() const;
	bool currentModeIsOnline() const;
	bool isNonOnline() const;
	char getPlayerSide() const;
	bool isMatchRunning() const;
	bool isTrainingMode() const;
	bool freezeGame = false;
	bool slowmoGame = false;
	bool allowNextFrame = false;
private:
	class HookHelp {
		friend class Game;
		void actualGameLoopHook(int param1);
	};
	bool sigscanFrameByFraming();
	void hookFrameByFraming();
	void static __cdecl updateAnimationsHookStatic(int param1, int param2, int param3, int param4);
	void updateAnimationsHook(int param1, int param2, int param3, int param4);
	void actualGameLoopHook(char* thisArg, int param1);
	void static __cdecl gameLoopHookStatic(int param1, int param2, int param3, int param4);
	void gameLoopHook(int param1, int param2, int param3, int param4);
	void gameLoopHookEmpty();
	gameLoop_t orig_gameLoop = nullptr;
	std::mutex orig_gameLoopMutex;
	trainingHudTick_t trainingHudTick = nullptr;
	getTrainingHudArgument_t getTrainingHudArgument = nullptr;
	actualGameLoop_t orig_actualGameLoop = nullptr;
	std::mutex orig_actualGameLoopMutex;
	updateAnimations_t orig_updateAnimations = nullptr;
	std::mutex orig_updateAnimationsMutex;
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
};

extern Game game;
