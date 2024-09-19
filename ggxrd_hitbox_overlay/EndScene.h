#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <atlbase.h>
#include <vector>
#include "Entity.h"
#include <condition_variable>
#include <mutex>
#include "DrawTextWithIconsParams.h"

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
using SendUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using ReadUnrealPawnData_t = void(__thiscall*)(char* thisArg);
using drawTextWithIcons_t = void(*)(DrawTextWithIconsParams* param_1, int param_2, int param_3, int param_4, int param_5, int param_6);

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device);
HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	HRESULT presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	void endSceneHook(IDirect3DDevice9* device);
	LRESULT WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void setPresentFlag();
	bool consumePresentFlag();
	void logic();
	void assignNextId(bool acquireLock = false);
	void onAswEngineDestroyed();
	void onHitDetectionStart();
	void onHitDetectionEnd();
	EndScene_t orig_EndScene = nullptr;
	std::mutex orig_EndSceneMutex;
	Present_t orig_Present = nullptr;
	std::mutex orig_PresentMutex;
	SendUnrealPawnData_t orig_SendUnrealPawnData = nullptr;
	std::mutex orig_SendUnrealPawnDataMutex;
	bool orig_SendUnrealPawnDataMutexLocked = false;
	DWORD orig_SendUnrealPawnDataMutexThreadId = NULL;
	ReadUnrealPawnData_t orig_ReadUnrealPawnData = nullptr;
	std::mutex orig_ReadUnrealPawnDataMutex;
	WNDPROC orig_WndProc = nullptr;
	std::mutex orig_WndProcMutex;
	bool orig_WndProcMutexLockedByWndProc = false;
	DWORD wndProcThread = 0;
	bool butDontPrepareBoxData = false;
	void(__thiscall* orig_drawTrainingHud)(char* thisArg) = nullptr;  // type is defined in Game.h: trainingHudTick_t
	std::mutex orig_drawTrainingHudMutex;
private:
	void processKeyStrokes();
	void clearContinuousScreenshotMode();
	void actUponKeyStrokesThatAlreadyHappened();
	class HookHelp {
		friend class EndScene;
		void sendUnrealPawnDataHook();
		void readUnrealPawnDataHook();
		void drawTrainingHudHook();
	};
	void sendUnrealPawnDataHook(char* thisArg);
	void readUnrealPawnDataHook(char* thisArg);
	void drawTrainingHudHook(char* thisArg);
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
	std::vector<HiddenEntity>::iterator findHiddenEntity(const Entity& ent);
	bool needToTakeScreenshot = false;

	// The EndScene function is actually being called twice: once by GuiltyGear and one more time by the Steam overlay.
	// However, Present is only called once each frame. So we use the Present function to determine if the next EndScene
	// call should draw the boxes.
	bool presentCalled = true;

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
	int tensionGainMaxCombo[2] { 0 };
	int burstGainMaxCombo[2] { 0 };
	int tensionGainLastCombo[2] { 0 };
	int burstGainLastCombo[2] { 0 };
};

extern EndScene endScene;
