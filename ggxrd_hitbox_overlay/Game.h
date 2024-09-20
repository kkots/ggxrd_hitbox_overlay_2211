#pragma once
#include "gameModes.h"
#include <mutex>

extern const char** aswEngine;

enum ELevelTick {
	LEVELTICK_TimeOnly,
	LEVELTICK_ViewportsOnly,
	LEVELTICK_All
};

using levelTick_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using updateBattleOfflineVer_t = void(__thiscall*)(char* thisArg, int param1);
using trainingHudTick_t = void(__thiscall*)(char* thisArg);
using getTrainingHudArgument_t = char*(__cdecl*)(void);
using updateAnimations_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using destroyAswEngine_t = void(__cdecl*)(void);
using UWorld_Tick_t = void(__thiscall*)(void* thisArg, ELevelTick TickType, float DeltaSeconds);

class Game {
public:
	bool onDllMain();
	GameMode getGameMode() const;
	bool currentModeIsOnline() const;
	bool isNonOnline() const;
	char getPlayerSide() const;
	bool isMatchRunning() const;
	bool isTrainingMode() const;
	int getBurst(int team) const;
	bool freezeGame = false;
	bool slowmoGame = false;
	bool allowNextFrame = false;
	trainingHudTick_t trainingHudTick = nullptr;
	getTrainingHudArgument_t getTrainingHudArgument = nullptr;
	uintptr_t aswEngineTickCountOffset = 0;
private:
	class HookHelp {
		friend class Game;
		void updateBattleOfflineVerHook(int param1);
		void UWorld_TickHook(ELevelTick TickType, float DeltaSeconds);
	};
	bool sigscanFrameByFraming();
	void hookFrameByFraming();
	void static __cdecl updateAnimationsHookStatic(int param1, int param2, int param3, int param4);
	void updateAnimationsHook(int param1, int param2, int param3, int param4);
	void updateBattleOfflineVerHook(char* thisArg, int param1);
	void static __cdecl levelTickHookStatic(int param1, int param2, int param3, int param4);
	void levelTickHook(int param1, int param2, int param3, int param4);
	void levelTickHookEmpty();
	static void destroyAswEngineHook();
	levelTick_t orig_levelTick = nullptr;
	std::mutex orig_levelTickMutex;
	updateBattleOfflineVer_t orig_updateBattleOfflineVer = nullptr;
	std::mutex orig_updateBattleOfflineVerMutex;
	updateAnimations_t orig_updateAnimations = nullptr;
	std::mutex orig_updateAnimationsMutex;
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
	bool needToCallEndSceneLogic = false;
	uintptr_t burstOffset = 0;
	destroyAswEngine_t orig_destroyAswEngine = nullptr;
	std::mutex orig_destroyAswEngineMutex;
	UWorld_Tick_t orig_UWorld_Tick = nullptr;
	std::mutex orig_UWorld_TickMutex;
	void UWorld_TickHook(void* thisArg, ELevelTick TickType, float DeltaSeconds);
};

extern Game game;
