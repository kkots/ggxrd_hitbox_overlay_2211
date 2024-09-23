#pragma once
#include "gameModes.h"
#include "Entity.h"
#include <mutex>

extern const char** aswEngine;

enum ELevelTick {
	LEVELTICK_TimeOnly,
	LEVELTICK_ViewportsOnly,
	LEVELTICK_All
};

using TickActors_FDeferredTickList_FGlobalActorIterator_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using updateBattleOfflineVer_t = void(__thiscall*)(char* thisArg, int param1);
using trainingHudTick_t = void(__thiscall*)(char* thisArg);
using getTrainingHudArgument_t = char*(__cdecl*)(void);
using TickActorComponents_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using destroyAswEngine_t = void(__cdecl*)(void);
using UWorld_Tick_t = void(__thiscall*)(void* thisArg, ELevelTick TickType, float DeltaSeconds);
using drawExGaugeHUD_t = void(__thiscall*)(void* thisArg, int param_1);
using drawStunMash_t = void(__thiscall*)(void* pawn, float bar, BOOL withBar, BOOL withLever);
using UWorld_IsPaused_t = bool(__thiscall*)(void* UWorld);

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
	DWORD aswEngineTickCountOffset = 0;
	bool shutdown = false;
	DWORD drawExGaugeHUDOffset = 0;
	drawExGaugeHUD_t drawExGaugeHUD = nullptr;
	drawExGaugeHUD_t orig_drawExGaugeHUD = nullptr;
	std::mutex orig_drawExGaugeHUDMutex;
	DWORD cameraOffset = 0;
	DWORD REDGameInfo_BattleOffset = 0;
	DWORD REDHUD_BattleOffset = 0;
private:
	class HookHelp {
		friend class Game;
		void updateBattleOfflineVerHook(int param1);
		void UWorld_TickHook(ELevelTick TickType, float DeltaSeconds);
		void drawExGaugeHUDHook(int param_1);
		bool UWorld_IsPausedHook();
	};
	bool sigscanFrameByFraming();
	void hookFrameByFraming();
	void static __cdecl TickActorComponentsHookStatic(int param1, int param2, int param3, int param4);
	void TickActorComponentsHook(int param1, int param2, int param3, int param4);
	void updateBattleOfflineVerHook(char* thisArg, int param1);
	void static __cdecl TickActors_FDeferredTickList_FGlobalActorIteratorHookStatic(int param1, int param2, int param3, int param4);
	void TickActors_FDeferredTickList_FGlobalActorIteratorHook(int param1, int param2, int param3, int param4);
	void TickActors_FDeferredTickList_FGlobalActorIteratorHookEmpty();
	static void destroyAswEngineHook();
	TickActors_FDeferredTickList_FGlobalActorIterator_t orig_TickActors_FDeferredTickList_FGlobalActorIterator = nullptr;
	std::mutex orig_TickActors_FDeferredTickList_FGlobalActorIteratorMutex;
	updateBattleOfflineVer_t orig_updateBattleOfflineVer = nullptr;
	std::mutex orig_updateBattleOfflineVerMutex;
	TickActorComponents_t orig_TickActorComponents = nullptr;
	std::mutex orig_TickActorComponentsMutex;
	const char** gameDataPtr = nullptr;
	const char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
	bool ignoreAllCallsButEarlier = false;
	bool needToCallEndSceneLogic = false;
	uintptr_t burstOffset = 0;
	destroyAswEngine_t orig_destroyAswEngine = nullptr;
	std::mutex orig_destroyAswEngineMutex;
	UWorld_Tick_t orig_UWorld_Tick = nullptr;
	std::mutex orig_UWorld_TickMutex;
	void UWorld_TickHook(void* thisArg, ELevelTick TickType, float DeltaSeconds);
	int float_max_bytes = 0x4f800000;
	float float_max = (float&)float_max_bytes;
	void drawStunLeverWithButtonMash(Entity pawn);
	void drawStunButtonMash(Entity pawn);
	drawStunMash_t drawStunMashPtr = nullptr;
	int IsPausedCallCount = 0;
	UWorld_IsPaused_t orig_UWorld_IsPaused = nullptr;
	std::mutex orig_UWorld_IsPausedMutex;
};

extern Game game;
