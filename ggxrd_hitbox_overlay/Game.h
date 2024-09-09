#pragma once
#include "gameModes.h"
#include <mutex>

extern const char** aswEngine;

using levelTick_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using updateBattleOfflineVer_t = void(__thiscall*)(char* thisArg, int param1);
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
		void updateBattleOfflineVerHook(int param1);
	};
	bool sigscanFrameByFraming();
	void hookFrameByFraming();
	void static __cdecl updateAnimationsHookStatic(int param1, int param2, int param3, int param4);
	void updateAnimationsHook(int param1, int param2, int param3, int param4);
	void updateBattleOfflineVerHook(char* thisArg, int param1);
	void static __cdecl levelTickHookStatic(int param1, int param2, int param3, int param4);
	void levelTickHook(int param1, int param2, int param3, int param4);
	void levelTickHookEmpty();
	levelTick_t orig_levelTick = nullptr;
	std::mutex orig_levelTickMutex;
	trainingHudTick_t trainingHudTick = nullptr;
	getTrainingHudArgument_t getTrainingHudArgument = nullptr;
	updateBattleOfflineVer_t orig_updateBattleOfflineVer = nullptr;
	std::mutex orig_updateBattleOfflineVerMutex;
	updateAnimations_t orig_updateAnimations = nullptr;
	std::mutex orig_updateAnimationsMutex;
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
	bool needToCallEndSceneLogic = false;
};

extern Game game;
