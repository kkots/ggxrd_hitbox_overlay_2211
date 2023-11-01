#pragma once
#include <d3d9.h>
#include <vector>
#include "Entity.h"
#include <condition_variable>
#include <mutex>
#include <atomic>
#include "MutexWhichTellsWhatThreadItsLockedBy.h"

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
using Present_t = HRESULT(__stdcall*)(IDirect3DDevice9*, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device);
HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);

class EndScene
{
public:
	bool onDllMain();
	bool onDllDetach();
	HRESULT presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
	void endSceneHook(IDirect3DDevice9* device);
	void endSceneHookUnloadingLogic(IDirect3DDevice9* device, bool* unloadingLogicTakesOver, HRESULT* returnThisInTheCaller);
	void setPresentFlag();
	bool consumePresentFlag();
	EndScene_t orig_EndScene = nullptr;
	MutexWhichTellsWhatThreadItsLockedBy orig_EndSceneMutex;
	Present_t orig_Present = nullptr;
	MutexWhichTellsWhatThreadItsLockedBy orig_PresentMutex;
private:
	bool isEntityAlreadyDrawn(const Entity& ent) const;
	void onDllDetachWhenEndSceneHooked();
	void onDllDetachWhenEndSceneNotHooked();

	// The EndScene function is actually being called twice: once by GuiltyGear and one more time by the Steam overlay.
	// However, Present is only called once each frame. So we use the Present function to determine if the next EndScene
	// call should draw the boxes.
	bool presentCalled = true;

	std::vector<Entity> drawnEntities;

	DWORD ongoingPresentCall = 0;
	bool presentMustReportEndSceneUnhooked = false;
	std::atomic_bool endSceneIsHooked = false;
	std::atomic_bool needUnhookAll = false;
	bool unhookedAll = false;
	bool endSceneUnhooked = false;
	std::condition_variable endSceneUnhookedConditionVariable;
	std::mutex endSceneUnhookedMutex;
};

extern EndScene endScene;
