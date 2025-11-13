#pragma once
#include "gameModes.h"
#include "Entity.h"
#include "InputRingBuffer.h"
#include <limits.h>
#include "DrawTextWithIconsParams.h"
#include "trainingSettings.h"
#include <vector>

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

enum DummyRecordingMode {
	DUMMY_MODE_IDLE,
	DUMMY_MODE_CONTROLLING,
	DUMMY_MODE_RECORDING,
	DUMMY_MODE_PLAYING_BACK,
	DUMMY_MODE_SETTING_CONTROLLER,
	DUMMY_MODE_CONTROLLER,
	DUMMY_MODE_COM,
	DUMMY_MODE_CONTINUOUS_ATTACKS
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
using drawJackoHouseHp_t = void(__thiscall*)(void* pawn);
using getGameViewportClient_t = void*(__thiscall*)(void* REDHUD);
using getExtraGameState_t = char*(__cdecl*)();
using getRiscForUI_t = float(__thiscall*)(char* thisArg);
using isStylish_t = BOOL(__thiscall*)(void*);
using setPositionResetType_t = void(__cdecl*)();
using getPlayerPadID_t = int(__cdecl*)();
using roundInit_t = void(__thiscall*)(void* thisArg);
using isGameModeNetwork_t = BOOL(__cdecl*)();
using drawTextureProbably_t = void(__cdecl*)(void* params);
using drawIcon_t = int (__cdecl*)(int iconIndex, DrawTextWithIconsParams* params, BOOL dontCommit);
using functionAfterSettingDelay_t = void (__thiscall*)(void* thisArg, int param1);
using trainingStructProcessPlayRecordReset_t = void (__thiscall*)(void* thisArg);
using functionInIsPlayInsideProcessPlayRecordReset_t = void* (__cdecl*)();
using appRealloc_t = void*(__cdecl*)(void* Ptr, int NewSize, int Alignment_unused);  // just pass 8 into alignment. Passing 0 for Ptr allocates new memory. Non-0 reallocates. Passing NewSize 0 frees the memory.
extern appRealloc_t appRealloc;

class Game {
public:
	bool onDllMain();
	bool sigscanAfterHitDetector();
	GameMode getGameMode() const;
	bool currentModeIsOnline() const;
	bool isNonOnline() const;
	char getPlayerSide() const;
	bool isMatchRunning() const;
	bool isTrainingMode() const;
	int getBurst(int team) const;
	bool isFading() const;
	bool isRoundend() const;
	DummyRecordingMode getDummyRecordingMode() const;
	int getDangerTime() const;
	bool bothPlayersHuman() const;
	void updateOnlineDelay();
	void onConnectionTierChanged();
	int currentPlayerControllingSide() const;
	bool freezeGame = false;
	bool allowNextFrame = false;
	trainingHudTick_t trainingHudTick = nullptr;  // the hook for this function is in EndScene.cpp
	getTrainingHud_t getTrainingHud = nullptr;
	DWORD aswEngineTickCountOffset = 0;
	DWORD dangerTimeOffset = 0;
	bool shutdown = false;
	DWORD drawExGaugeHUDOffset = 0;
	drawExGaugeHUD_t drawExGaugeHUD = nullptr;
	drawExGaugeHUD_t orig_drawExGaugeHUD = nullptr;
	DWORD cameraOffset = 0;
	DWORD REDGameInfo_BattleOffset = 0;
	DWORD REDHUD_BattleOffset = 0;
	DWORD roundendSuperfreezeCounterOffset = 0;
	DWORD aswEng0x1c710cOffset = 0;  // if I knew what this was I would give it a more sensible name. All I know is it runs code that helps decide what happens during round end and interludes
	BOOL& postEffectOn();
	bool isStylish(Entity pawn) const;
	int getStylishDefenseInverseModifier() const;
	int getStylishBurstGainModifier() const;
	int getHandicap(int playerIndex) const;
	int getTrainingSetting(TrainingSettingId setting) const;
	InputRingBuffer* getInputRingBuffers() const;
	DWORD matchInfoOffset = 0;
	int getMatchTimer() const;  // decrements at the start of logic tick
	char** gameDataPtr = nullptr;  // REDGameCommon
	BYTE* getStaticFont() const;
	int getPlayerPadID();
	setPositionResetType_t orig_setPositionResetType = nullptr;
	roundInit_t orig_roundInit = nullptr;
	// clears training HUD's input history
	void clearInputHistory();
	bool is0xa8PreparingCamera() const;
	static const char* formatGameMode(GameMode gameMode);
	int* currentMenu = nullptr;
	void** allMenus = nullptr;
	BOOL drawWinsHook(BYTE* rematchMenuByte);
	// used to store and update scores of rematch menus and is used to detect if a new showcasing of rematch menus has begun
	int prevScores[2];
	// whether this rematch menu has changed its score on this showcasing of the rematch menu. Gets updated during each showcasing of the rematch menus
	bool changedScore[2];
	// the last player side that was obtained through the rematch menus in online mode
	int lastRematchMenuPlayerSide = -1;
	void hideRankIcons();
	drawTextureProbably_t drawTextureProbably = nullptr;
	drawIcon_t drawIcon = nullptr;
	uintptr_t drawRankInLobbyOverPlayersHeads = 0;
	uintptr_t drawRankInLobbySearchMemberList = 0;
	uintptr_t drawRankInLobbyMemberList_NonCircle = 0;
	uintptr_t drawRankInLobbyMemberList_Circle = 0;
	void onUsePositionResetChanged();
	std::vector<void*> actorsToAllowTickFor;
	void allowTickForActor(void* actor);
	const char* readFName(int fname, bool* isWide);
	bool sigscanFNamesAndAppRealloc();
	void onFPSChanged();
	// first check if the game mode is network at all, and that you're in a match (although it would also tell if you're just in the lobby)
	bool isOnlineTrainingMode_Part() const;
private:
	getPlayerPadID_t getPlayerPadIDPtr = nullptr;
	class HookHelp {
		friend class Game;
		void updateBattleOfflineVerHook(int param1);
		void UWorld_TickHook(ELevelTick TickType, float DeltaSeconds);
		void drawExGaugeHUDHook(int param_1);
		bool UWorld_IsPausedHook();
		float getRiscForUI_BackgroundHook();
		float getRiscForUI_ForegroundHook();
		void roundInitHook();
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
	void roundInitHook(Entity pawn);
	
	static void setPositionResetTypeHookStatic();
	void setPositionResetTypeHook();
	TickActors_FDeferredTickList_FGlobalActorIterator_t orig_TickActors_FDeferredTickList_FGlobalActorIterator = nullptr;
	updateBattleOfflineVer_t orig_updateBattleOfflineVer = nullptr;
	TickActorComponents_t orig_TickActorComponents = nullptr;
	char** netplayStruct = nullptr;
	bool ignoreAllCalls = false;
	bool ignoreAllCallsButEvenEarlier = false;
	uintptr_t burstOffset = 0;
	destroyAswEngine_t orig_destroyAswEngine = nullptr;
	UWorld_Tick_t orig_UWorld_Tick = nullptr;
	void UWorld_TickHook(void* thisArg, ELevelTick TickType, float DeltaSeconds);
	int float_max_bytes = 0x4f800000;
	float float_max = (float&)float_max_bytes;
	void drawStunLeverWithButtonMash(Entity pawn);
	void drawStunButtonMash(Entity pawn);
	drawStunMash_t drawStunMashPtr = nullptr;
	int IsPausedCallCount = 0;
	UWorld_IsPaused_t orig_UWorld_IsPaused = nullptr;
	BYTE** inputsHolder = nullptr;
	enum PlayRecordEnum {
		PlayRecordEnum_Play,
		PlayRecordEnum_Record
	} isPlayRecord = PlayRecordEnum_Play;
	bool playRecordFired = false;
	bool playPressed[4] { false };
	bool recordPressed[4] { false };
	bool resetPressed[4] { false };
	bool unknownPressed[4] { false };
	bool buttonPressed(int padInd, bool isMenu, DWORD code);
	DWORD getPressedButtons(int padInd, bool isMenu);
	bool buttonHeld(int padInd, bool isMenu, DWORD code);
	DWORD getHeldButtons(int padInd, bool isMenu);
	void setButtonPressed(int padInd, bool isMenu, DWORD code);
	void setButtonNotPressed(int padInd, bool isMenu, DWORD code);
	void setButtonPressedNotPressed(int padInd, bool isMenu, DWORD code, bool isPressed);
	void onNeedRememberPress(int padInd, bool* pressed, ButtonCodeMenu code);
	void onNeedRememberPress(int padInd, bool* pressed, ButtonCode code);
	void onNeedRememberPress(int padInd, bool* pressed, bool isMenu, DWORD code);
	bool onNeedForcePress(int padInd, bool* pressed, ButtonCodeMenu code);
	bool onNeedForcePress(int padInd, bool* pressed, ButtonCode code);
	bool onNeedForcePress(int padInd, bool* pressed, bool isMenu, DWORD code);
	inline int getMainQuadrant() const { if (*gameDataPtr) return 0; return *(int*)(*gameDataPtr + 0x140); }
	drawJackoHouseHp_t drawJackoHouseHp = nullptr;
	BOOL* postEffectOnPtr = nullptr;
	getRiscForUI_t orig_getRiscForUI_Background = nullptr;
	getRiscForUI_t orig_getRiscForUI_Foreground = nullptr;
	isStylish_t isStylishPtr = nullptr;
	uintptr_t stylishDefenseInverseModifierOffset = 0;
	uintptr_t stylishBurstGainModifierOffset = 0;
	uintptr_t handicapsOffset = 0;
	getTrainingSetting_t getTrainingSettingPtr = nullptr;
	uintptr_t inputRingBuffersOffset = 0;
	int lastSavedPositionX[2] { -252000, 252000 };
	DWORD cameraCenterXOffset = 0;
	int numberOfPlayersReset = 0;
	uintptr_t normal0xa8ElementVtable = 0;
	int getRematchMenuPlayerSide() const;
	bool isRematchMenuOpen() const;
	isGameModeNetwork_t isGameModeNetwork = nullptr;
	static int hiddenRankDrawIcon(int iconIndex, DrawTextWithIconsParams* params, BOOL dontCommit);
	static void hiddenRankDrawTextureProbably(void* params);
	void patchDrawIcon(uintptr_t addr);
	void patchDrawTextureProbably(uintptr_t addr);
	struct OnlineDelaySettings {
		bool firstUseEver = true;
		bool fullscreen = false;
		int delayToSet = 1;
		inline bool operator==(const OnlineDelaySettings& other) const {
			return firstUseEver == other.firstUseEver
				&& fullscreen == other.fullscreen
				&& delayToSet == other.delayToSet;
		}
		inline bool operator!=(const OnlineDelaySettings& other) const {
			return !(*this == other);
		}
	} onlineDelayLastSet;
	bool sigscanAndHookPositionResetAndGetPlayerPadID();
	bool attemptedToSigscanPositionReset = false;
	bool attemptedToHookPositionReset = false;
	bool sigscanTrainingStructProcessPlayRecordReset();
	trainingStructProcessPlayRecordReset_t trainingStructProcessPlayRecordReset = nullptr;
	bool attemptedToSigscanPlayedPadID = false;
	bool attemptedToSigscanTrainingStructProcessPlayRecordReset = false;
	int playOrRecordPressCounter = INT_MAX;
	functionInIsPlayInsideProcessPlayRecordReset_t functionInIsPlayInsideProcessPlayRecordReset = nullptr;
	static void* functionInIsPlayInsideProcessPlayRecordResetHook();
	bool doNotIncrementSlotInputsIndex = false;
	struct FNameEntry {
		unsigned long long Flags;
		int Index;
		FNameEntry* HashNext;
		BYTE data[1];
	};
	typedef FNameEntry** ArrayOfFNameEntryPointers;
	ArrayOfFNameEntryPointers* fnameNamesPtr = nullptr;
	bool attemptedToFindFNameNames = false;
	bool attemptedToSwapOutFPS = false;
	bool swapOutFPS();
};

extern Game game;
