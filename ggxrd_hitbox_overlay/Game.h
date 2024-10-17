#pragma once
#include "gameModes.h"
#include "Entity.h"
#include <mutex>

extern char** aswEngine;

enum ELevelTick {
	LEVELTICK_TimeOnly,
	LEVELTICK_ViewportsOnly,
	LEVELTICK_All
};

enum ButtonCode {
	BUTTON_CODE_UP = 0x1,
	BUTTON_CODE_DOWN = 0x2,
	BUTTON_CODE_LEFT = 0x4,
	BUTTON_CODE_RIGHT = 0x8,
	BUTTON_CODE_PUNCH = 0x10,
	BUTTON_CODE_KICK = 0x20,
	BUTTON_CODE_SLASH = 0x40,
	BUTTON_CODE_HSLASH = 0x80,
	BUTTON_CODE_DUST = 0x100,
	BUTTON_CODE_TAUNT = 0x200,
	BUTTON_CODE_SPECIAL = 0x400,
	BUTTON_CODE_PK_MACRO = 0x800,
	BUTTON_CODE_RC_MACRO = 0x1000,
	BUTTON_CODE_IK_MACRO = 0x2000,
	BUTTON_CODE_BLITZ_MACRO = 0x4000,
	BUTTON_CODE_BURST_MACRO = 0x8000,
	BUTTON_CODE_PLAY = 0x10000,
	BUTTON_CODE_RECORD = 0x20000
};

// Button codes for menus
enum ButtonCodeMenu {
	BUTTON_CODE_MENU_UP = 0x1,
	BUTTON_CODE_MENU_RIGHT = 0x2,
	BUTTON_CODE_MENU_DOWN = 0x4,
	BUTTON_CODE_MENU_LEFT = 0x8,
	BUTTON_CODE_MENU_RCODE = 0x1000,
	BUTTON_CODE_MENU_MENU = 0x2000,
	BUTTON_CODE_MENU_DECIDE = 0x4000,
	BUTTON_CODE_MENU_BACK = 0x8000,
	BUTTON_CODE_MENU_PageChange2 = 0x10000,
	BUTTON_CODE_MENU_CameraControl2 = 0x20000,
	BUTTON_CODE_MENU_PageChange1 = 0x40000,
	BUTTON_CODE_MENU_CameraControl1 = 0x80000,
	BUTTON_CODE_MENU_PAUSE = 0x400000,
	BUTTON_CODE_MENU_RESET = 0x800000,
	BUTTON_CODE_MENU_UNKNOWN = 0x1000000  // functions same way as Reset or Backspace
};

using TickActors_FDeferredTickList_FGlobalActorIterator_t = void(__cdecl*)(int param1, int param2, int param3, int param4);
using updateBattleOfflineVer_t = void(__thiscall*)(char* thisArg, int param1);
using trainingHudTick_t = void(__thiscall*)(char* thisArg);
using getTrainingHud_t = char*(__cdecl*)(void);
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
	trainingHudTick_t trainingHudTick = nullptr;  // the hook for this function is in EndScene.cpp
	getTrainingHud_t getTrainingHud = nullptr;
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
	char** gameDataPtr = nullptr;
	char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
	bool ignoreAllCallsButEarlier = false;
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
	BYTE** inputsHolder = nullptr;
	bool recordPressed[4] { false };
	bool playPressed[4] { false };
	bool resetPressed[4] { false };
	bool unknownPressed[4] { false };
	bool buttonPressed(int padInd, bool isMenu, DWORD code);
	bool buttonHeld(int padInd, bool isMenu, DWORD code);
	void setButtonPressed(int padInd, bool isMenu, DWORD code);
	void onNeedRememberPress(int padInd, bool* pressed, ButtonCodeMenu code);
	void onNeedRememberPress(int padInd, bool* pressed, ButtonCode code);
	void onNeedRememberPress(int padInd, bool* pressed, bool isMenu, DWORD code);
	void onNeedForcePress(int padInd, bool* pressed, ButtonCodeMenu code);
	void onNeedForcePress(int padInd, bool* pressed, ButtonCode code);
	void onNeedForcePress(int padInd, bool* pressed, bool isMenu, DWORD code);
};

extern Game game;
