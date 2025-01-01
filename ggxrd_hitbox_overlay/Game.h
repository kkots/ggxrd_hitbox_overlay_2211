#pragma once
#include "gameModes.h"
#include "Entity.h"
#include "InputRingBuffer.h"

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

enum TrainingSettingId {
	TRAINING_SETTING_DISPLAY_SETTINGS_ENEMY_STATUS=0 /* TRP_DispDummy */,
    TRAINING_SETTING_DISPLAY_SETTINGS_DAMAGE_INFO=1 /* TRP_DispDamage */,
    TRAINING_SETTING_DISPLAY_SETTINGS_INPUT_HITSTORY=2 /* TRP_DispLog */,
    TRAINING_SETTING_DISPLAY_SETTINGS_VIRTUAL_STICK=3 /* TRP_DispVirtualLever */,
    TRAINING_SETTING_DISPLAY_SETTINGS_VIRTUAL_STICKS_POSITION=4 /* TRP_VirtualLeverPos */,
    TRAINING_SETTING_HP_REGEN=5 /* TRP_HitPoint */,
    TRAINING_SETTING_PLAYER_1_MAX_HP=6 /* TRP_HitPointMax1P */,
    TRAINING_SETTING_PLAYER_2_MAX_HP=7 /* TRP_HitPointMax2P */,
    TRAINING_SETTING_TENSION_GAUGE=8 /* TRP_TensionInitial */,
    TRAINING_SETTING_PSYCH_BURST=9 /* TRP_BurstStock */,
    TRAINING_SETTING_RISC_LEVEL=10 /* TRP_GuardBalance */,
    TRAINING_SETTING_COUNTER_HIT=11 /* TRP_CounterHit */,
    TRAINING_SETTING_DANGER_TIME=12 /* TRP_DangerTime */,
    TRAINING_SETTING_STUN=13 /* TRP_Kizetsu */,
    TRAINING_SETTING_ENEMY_STATUS=14 /* TRP_Enemy */,
    TRAINING_SETTING_COM_LEVEL=15 /* TRP_CPULevel */,
    TRAINING_SETTING_SLOT_NUMBER=16 /* TRP_MemorySlot */,
    TRAINING_SETTING_STANCE=17 /* TRP_Posing */,
    TRAINING_SETTING_BLOCK_SETTINGS=18 /* TRP_Guard */,
    TRAINING_SETTING_BLOCK_SWITCHING=19 /* TRP_GuardSwitch */,
    TRAINING_SETTING_BLOCK_TYPE=20 /* TRP_GuardType */,
    TRAINING_SETTING_RECOVERY=21 /* TRP_AirUkemi */,
    TRAINING_SETTING_STAGGER_RECOVERY=22 /* TRP_JitabataRecover */,
    TRAINING_SETTING_STUN_RECOVERY=23 /* TRP_KizetsuRecover */,
    TRAINING_SETTING_AUTO_PSYCH_BURST=24 /* TRP_AutoBurst */,
    TRAINING_SETTING_INPUT_LATENCY_1P=25 /* TRP_Delay1P */,
    TRAINING_SETTING_INPUT_LATENCY_2P=26 /* TRP_Delay2P */,
    TRAINING_SETTING_CHAR_SPECIFIC_SOL_DRAGON_INSTALL=27 /* TRP_SOL */,
    TRAINING_SETTING_CHAR_SPECIFIC_MILLIA_SILENT_FORCE=28 /* TRP_MLL */,
    TRAINING_SETTING_CHAR_SPECIFIC_ZATO_EDDIE_GAUGE=29 /* TRP_ZAT */,
    TRAINING_SETTING_CHAR_SPECIFIC_CHIPP_SHURIKEN=30 /* TRP_CHP */,
    TRAINING_SETTING_CHAR_SPECIFIC_FAUST_WHAT_COULD_THIS_BE=31 /* TRP_FAU */,
    TRAINING_SETTING_CHAR_SPECIFIC_VENOM_BISHOP_RUNOUT=32 /* TRP_VEN */,
    TRAINING_SETTING_CHAR_SPECIFIC_SIN_CALORIE_GAUGE=33 /* TRP_SIN */,
    TRAINING_SETTING_CHAR_SPECIFIC_JOHNNY_GLITTER_IS_GOLD=34 /* TRP_JHN0 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JOHNNY_MIST_FINER=35 /* TRP_JHN1 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JACKO_P_GHOST=36 /* TRP_JKO0 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JACKO_K_GHOST=37 /* TRP_JKO1 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JACKO_S_GHOST=38 /* TRP_JKO2 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JACKO_SKILL_COOLDOWN=39 /* TRP_JKO3 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JAM_K_ASANAGI_NO_KOKYUU=40 /* TRP_JAM0 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JAM_S_ASANAGI_NO_KOKYUU=41 /* TRP_JAM1 */,
    TRAINING_SETTING_CHAR_SPECIFIC_JAM_H_ASANAGI_NO_KOKYUU=42 /* TRP_JAM2 */,
    TRAINING_SETTING_CHAR_SPECIFIC_RAVEN_EXCITEMENT=43 /* TRP_RVN */,
    TRAINING_SETTING_DISPLAY_COMBO_RECIPE_RECORD=44 /* TRP_DISP_ActRecord */,
    TRAINING_SETTING_DISPLAY_COMBO_RECIPE_RECIPE=45 /* TRP_DISP_ActRecipe */
};

enum TrainingSettingValue_CounterHit {
	TRAINING_SETTING_VALUE_COUNTER_HIT_DEFAULT,
	TRAINING_SETTING_VALUE_COUNTER_HIT_FORCED,
	TRAINING_SETTING_VALUE_COUNTER_HIT_RANDOM,
	TRAINING_SETTING_VALUE_COUNTER_HIT_FORCED_MORTAL_COUNTER
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
using getTrainingSetting_t = int(__thiscall*)(void* trainingStruct, TrainingSettingId setting, BOOL outsideTraining);

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
	bool freezeGame = false;
	bool slowmoGame = false;
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
	BOOL& postEffectOn();
	bool isStylish(Entity pawn) const;
	int getStylishDefenseInverseModifier() const;
	int getStylishBurstGainModifier() const;
	int getHandicap(int playerIndex) const;
	int getTrainingSetting(TrainingSettingId setting) const;
	InputRingBuffer* getInputRingBuffers() const;
	DWORD matchInfoOffset = 0;
	int getMatchTimer() const;  // decrements at the start of logic tick
private:
	class HookHelp {
		friend class Game;
		void updateBattleOfflineVerHook(int param1);
		void UWorld_TickHook(ELevelTick TickType, float DeltaSeconds);
		void drawExGaugeHUDHook(int param_1);
		bool UWorld_IsPausedHook();
		float getRiscForUI_BackgroundHook();
		float getRiscForUI_ForegroundHook();
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
	updateBattleOfflineVer_t orig_updateBattleOfflineVer = nullptr;
	TickActorComponents_t orig_TickActorComponents = nullptr;
	char** gameDataPtr = nullptr;
	char** playerSideNetworkHolder = nullptr;
	unsigned slowmoSkipCounter = 0;
	bool ignoreAllCalls = false;
	bool ignoreAllCallsButEarlier = false;
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
};

extern Game game;
