#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>
#include <vector>
#include "Entity.h"
#include <condition_variable>
#include <mutex>
#include "DrawTextWithIconsParams.h"
#include "PlayerInfo.h"

using SendUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using ReadUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using drawTextWithIcons_t = void(*)(DrawTextWithIconsParams* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);
using endSceneCaller_t = void(__thiscall*)(void* thisArg, int param1, int param2, int param3);
using BBScr_createObjectWithArg_t = void(__thiscall*)(void* pawn, char* animName, unsigned int posType);

LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void logic();
	void assignNextId(bool acquireLock = false);
	void onAswEngineDestroyed();
	void onHitDetectionStart(int hitDetectionType);
	void onHitDetectionEnd(int hitDetectionType);
	void onUWorld_TickBegin();
	void onUWorld_Tick();
	void endSceneHook(IDirect3DDevice9* device);
	void registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender);
	bool didHit(Entity attacker);
	SendUnrealPawnData_t orig_SendUnrealPawnData = nullptr;
	std::mutex orig_SendUnrealPawnDataMutex;
	bool orig_SendUnrealPawnDataMutexLocked = false;
	DWORD orig_SendUnrealPawnDataMutexThreadId = NULL;
	ReadUnrealPawnData_t orig_ReadUnrealPawnData = nullptr;
	std::mutex orig_ReadUnrealPawnDataMutex;
	WNDPROC orig_WndProc = nullptr;
	std::mutex orig_WndProcMutex;
	bool orig_WndProcMutexLockedByWndProc = false;
	bool butDontPrepareBoxData = false;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	std::mutex orig_drawTrainingHudMutex;
	endSceneCaller_t orig_endSceneCaller = nullptr;
	std::mutex orig_endSceneCallerMutex;
	BBScr_createObjectWithArg_t orig_BBScr_createObjectWithArg = nullptr;
	std::mutex orig_BBScr_createObjectWithArgMutex;
	
	PlayerInfo players[2] { 0 };
	std::vector<ProjectileInfo> projectiles;
	DWORD logicThreadId = NULL;
private:
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void sendUnrealPawnDataHook();
		void readUnrealPawnDataHook();
		void drawTrainingHudHook();
		void endSceneCallerHook(int param1, int param2, int param3);
		void BBScr_createObjectWithArgHook(char* animName, unsigned int posType);
	};
	void sendUnrealPawnDataHook(char* thisArg);
	void readUnrealPawnDataHook(char* thisArg);
	void drawTrainingHudHook(char* thisArg);
	void BBScr_createObjectWithArgHook(Entity pawn, char* animName, unsigned int posType);
	void prepareDrawData(bool* needClearHitDetection);
	struct HiddenEntity {
		Entity ent{ nullptr };
		int scaleX = 0;
		int scaleY = 0;
		int scaleZ = 0;
		int scaleDefault = 0;
		bool wasFoundOnThisFrame = false;
	};
	bool isEntityAlreadyDrawn(const Entity& ent) const;
	void noGravGifMode();
	void hideEntity(Entity ent);
	std::vector<HiddenEntity>::iterator findHiddenEntity(const Entity& ent);
	bool needToTakeScreenshot = false;

	std::vector<Entity> drawnEntities;

	std::vector<HiddenEntity> hiddenEntities;
	unsigned int allowNextFrameBeenHeldFor = 0;
	unsigned int allowNextFrameCounter = 0;
	bool freezeGame = false;
	bool continuousScreenshotMode = false;
	bool needContinuouslyTakeScreens = false;
	unsigned int p1PreviousTimeOfTakingScreen = ~0;
	unsigned int p2PreviousTimeOfTakingScreen = ~0;
	
	drawTextWithIcons_t drawTextWithIcons = nullptr;
	
	bool needToRunNoGravGifMode = false;
	void drawTexts();
	
	uintptr_t superflashInstigatorOffset = 0;
	uintptr_t superflashCounterAllOffset = 0;
	uintptr_t superflashCounterSelfOffset = 0;
	Entity getSuperflashInstigator();
	int getSuperflashCounterAll();
	
	bool measuringFrameAdvantage = false;
	int measuringLandingFrameAdvantage = -1;
	void restartMeasuringFrameAdvantage(int index);
	void restartMeasuringLandingFrameAdvantage(int index);
	
	int tensionRecordedHit[2] { 0 };
	int burstRecordedHit[2] { 0 };
	int tensionGainOnLastHit[2] { 0 };
	bool tensionGainOnLastHitUpdated[2] { 0 };
	int burstGainOnLastHit[2] { 0 };
	bool burstGainOnLastHitUpdated[2] { 0 };
	
	DWORD prevAswEngineTickCount = 0;
	bool drawDataPrepared = false;
	bool shutdown = false;
	HANDLE shutdownFinishedEvent = NULL;
	struct RegisteredHit {
		ProjectileInfo projectile;
		HitResult hitResult;
		Entity attacker;
		Entity defender;
		bool hasHitbox;
		bool isPawn;
	};
	std::vector<RegisteredHit> registeredHits;
};

extern EndScene endScene;
