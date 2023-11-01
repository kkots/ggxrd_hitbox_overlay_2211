#include "pch.h"
#include "EndScene.h"
#include "Direct3DVTable.h"
#include "Detouring.h"
#include "DrawOutlineCallParams.h"
#include "AltModes.h"
#include "HitDetector.h"
#include "logging.h"
#include "Game.h"
#include "EntityList.h"
#include "InvisChipp.h"
#include "Graphics.h"
#include "Camera.h"
#include <algorithm>
#include "collectHitboxes.h"
#include "Throws.h"
#include "colors.h"
#include <chrono>

using namespace std::literals;

EndScene endScene;

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device) {
	if (endScene.consumePresentFlag()) {

		bool unloadingLogicTakesOver;
		HRESULT returnThisInTheCaller;
		endScene.endSceneHookUnloadingLogic(device , &unloadingLogicTakesOver, &returnThisInTheCaller);
		if (unloadingLogicTakesOver) {
			return returnThisInTheCaller;
		}

		bool needToClearHitDetection = false;
		if (*aswEngine == nullptr) {
			needToClearHitDetection = true;
		} else if (altModes.isGameInNormalMode(&needToClearHitDetection)) {
			if (!(game.isMatchRunning() ? true : altModes.roundendCameraFlybyType() != 8)) {
				needToClearHitDetection = true;
			} else {
				endScene.endSceneHook(device);
			}
		}
		if (needToClearHitDetection) {
			hitDetector.clearAllBoxes();
		}
	}
	MutexWhichTellsWhatThreadItsLockedByGuard guard(endScene.orig_EndSceneMutex);
	return endScene.orig_EndScene(device);
}

HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	return endScene.presentHook(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT EndScene::presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	setPresentFlag();
	ongoingPresentCall = GetCurrentThreadId();
	MutexWhichTellsWhatThreadItsLockedByGuard guard(endScene.orig_PresentMutex);
	HRESULT result = orig_Present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);  // may call d3d9.dll::EndScene() (and, consecutively, the hook)
	ongoingPresentCall = 0;

	if (presentMustReportEndSceneUnhooked) {
		endSceneUnhookedMutex.lock();
		endSceneUnhooked = true;
		logwrap(fputs("EndScene::presentHook(...): endSceneUnhooked set to true\n", logfile));
		endSceneUnhookedConditionVariable.notify_all();
		endSceneUnhookedMutex.unlock();  // race condition against DLL unloading procedure starts on this line
	}

	return result;
}

bool EndScene::onDllMain() {
	orig_EndScene = (EndScene_t)direct3DVTable.getDirect3DVTable()[42];
	orig_Present = (Present_t)direct3DVTable.getDirect3DVTable()[17];

	if (!detouring.attach(&(PVOID&)(orig_EndScene),
		hook_EndScene,
		&orig_EndSceneMutex,
		"EndScene")) return false;

	if (!detouring.attach(&(PVOID&)(orig_Present),
		hook_Present,
		&orig_PresentMutex,
		"Present")) return false;
	
	return true;
}

bool EndScene::consumePresentFlag() {
	if (!presentCalled) return false;
	presentCalled = false;
	return true;
}

void EndScene::setPresentFlag() {
	presentCalled = true;
}

bool EndScene::isEntityAlreadyDrawn(const Entity& ent) const {
	return std::find(drawnEntities.cbegin(), drawnEntities.cend(), ent) != drawnEntities.cend();
}

void EndScene::endSceneHook(IDirect3DDevice9* device) {
	logOnce(fputs("endSceneHook called\n", logfile));
	entityList.populate();
	logOnce(fputs("entityList.populate() called\n", logfile));
	if (!entityList.areAnimationsNormal()) {
		hitDetector.clearAllBoxes();
		#ifdef LOG_PATH
		didWriteOnce = true;
		#endif
		return;
	}
	invisChipp.onEndSceneStart();
	logOnce(fputs("invisChipp.onEndSceneStart() called\n", logfile));
	graphics.onEndSceneStart(device);
	logOnce(fputs("graphics.onEndSceneStart() called\n", logfile));
	drawOutlineCallParamsManager.onEndSceneStart();
	logOnce(fputs("drawOutlineCallParamsManager.onEndSceneStart() called\n", logfile));
	camera.onEndSceneStart();
	logOnce(fputs("camera.onEndSceneStart() called\n", logfile));
	drawnEntities.clear();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	for (auto i = 0; i < entityList.count; i++)
	{
		Entity ent(entityList.list[i]);
		if (isEntityAlreadyDrawn(ent)) continue;

		bool active = ent.isActive();
		logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));

		if (invisChipp.needToHide(ent)) continue;
		DrawHitboxArrayCallParams hurtbox;
		collectHitboxes(ent, active, &hurtbox, &graphics.hitboxes, &graphics.points, &graphics.pushboxes);
		HitDetector::WasHitInfo wasHitResult = hitDetector.wasThisHitPreviously(ent, hurtbox);
		if (!wasHitResult.wasHit) {
			graphics.hurtboxes.push_back({false, hurtbox});
		} else {
			graphics.hurtboxes.push_back({true, hurtbox, wasHitResult.hurtbox});
		}
		logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
		drawnEntities.push_back(ent);
		logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

		// Attached entities like dusts
		const auto attached = *(char**)(ent + 0x204);
		if (attached != nullptr) {
			logOnce(fprintf(logfile, "Attached entity: %p\n", attached));
			collectHitboxes(attached, active, &hurtbox, &graphics.hitboxes, &graphics.points, &graphics.pushboxes);
			graphics.hurtboxes.push_back({false, hurtbox});
			drawnEntities.push_back(attached);
		}
	}

	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));

	graphics.drawAll();
	logOnce(fputs("graphics.drawAll() call successful\n", logfile));

#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}

bool EndScene::onDllDetach() {
	logwrap(fputs("EndScene::onDllDetach() called\n", logfile));
	if (endSceneIsHooked) onDllDetachWhenEndSceneHooked();
	else onDllDetachWhenEndSceneNotHooked();
	return true;
}

void EndScene::onDllDetachWhenEndSceneNotHooked() {
	logwrap(fputs("EndScene::onDllDetachWhenEndSceneNotHooked() called\n", logfile));
	detouring.detachAll();
	Sleep(100);  // wait for hooks other than EndScene to return for sure
}

void EndScene::onDllDetachWhenEndSceneHooked() {
	logwrap(fputs("EndScene::onDllDetachWhenEndSceneHooked() called\n", logfile));
	needUnhookAll = true;
	{
		std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
		std::unique_lock<std::mutex> guard(endSceneUnhookedMutex);
		while (true) {
			endSceneUnhookedConditionVariable.wait_for(guard, 500ms);  // when the game is closing it starts unloading all DLLs, but EndScene is no longer running
			if (endSceneUnhooked) {
				break;
			}
			std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
			if (currentTime - startTime >= 500ms) {
				onDllDetachWhenEndSceneNotHooked();
				break;
			}
		}
	}
	logwrap(fputs("EndScene::onDllDetachWhenEndSceneHooked saw that endSceneUnhooked is true\n", logfile));
	Sleep(100);  // wait for endSceneHookUnloadingLogic and hook_EndScene to return completely
	return;
}

// The way unloading logic works is the following:
// Instead of doing all this you can just call FreeLibrary, passing it a handle that you got from DllMain and call it a day.
// It will "unload" the DLL, call its DllMain function with reason DLL_PROCESS_DETACH, where you can unhook all of your hooks.
// There are numerous problems with that:
// Problem 1) The DLL doesn't actually unload. If you change the DLL on the disk and load it again via thread injection,
//            it will still use the old DLL unless you restart the whole process.
// Problem 2) It's practically impossible, but possible in theory that between the time you read orig_something
//            pointer to an original function and between the call through that pointer, you unhook all your hooks
//            and the pointer changes. Calling through the old pointer to the original will lead to a crash.
// Problem 3) It's practically impossible, but possible in theory that when unhooking all your hooks some of them
//            will be executing the region where the jump to your hook happens from the original-original function or where
//            the hooked-original function jumps to your hook. Detours library has a way of mitigating that but
//            it must be provided specific thread handles in its DetourUpdateThread(...) function for that to happen.
// If we can ignore Problem 2) and Problem 3), because that never happens, then to solve Problem 1) you need to call
// FreeLibrary passing it the address of the image base of the DLL in memory. Then it will call DllMain function
// with reason DLL_PROCESS_DETACH and then, once that returns, unload the DLL for real.
// This is actually dangerous because it unmaps all its memory pages, so if any DLL function, including your hooks,
// is still executing at that moment, the process will crash.
// 
// So why not unhook all hooks and then (possibly a Sleep(...) call and then) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FreeLibrary, &__ImageBase, 0, NULL)?
// This will add a little delay between your final function exiting and the created thread unloading the DLL,
// so all hooks currently running will have time to exit before the memory is unmapped.
// 
// Well I guess you could do that and it would work fine 100% of the time.
// But this wouldn't solve Problem 2) and Problem 3) (I'm just doing this out of principle now).
// However, the solution described below still relies basically on waiting long enough for hooks to return.
// 
// First, we start with DLL already injected using the injector and EndScene and determineHitType successfully hooked.
// Then, the user calls injector again to unload the DLL, which injects a thread into FreeLibrary which calls
// DllMain with reason DLL_PROCESS_DETACH.
// 
// The DllMain will call endScene.onDllDetach() which will tell EndScene() hook to, next time it's called, unhook all functions.
// The EndScene() hook sees the command and unhooks everything and sets a flag for itself to that when it runs next time,
// it finally unhooks itself and reports to the thread waiting at endScene.onDllDetach() that it has finished with all the unhooking.
// The two-step unhooking is necessary as a wait mechanism to wait for the determineHitType hook to exit.
// This relies on determineHitType hook taking less time to run than the duration of one frame.
// However, after determineHitType hook runs one last time it will never run again, because it got unhooked on the previous frame.
// So on the next frame after that we only need to unhook EndScene.
// The time between EndScene() reporting to endScene.onDllDetach() and EndScene() returning must be short so that
// the DLL doesn't get unloaded while EndScene() is still running.
// To ensure that, endScene.onDllDetach() also wait 100ms extra.
void EndScene::endSceneHookUnloadingLogic(IDirect3DDevice9* device, bool* unloadingLogicTakesOver, HRESULT* returnThisInTheCaller) {
	*unloadingLogicTakesOver = false;
	endSceneIsHooked = true;

	if (needUnhookAll) {
		logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...) is kicking in\n", logfile));
		logwrap(fprintf(logfile, "EndScene::endSceneHookUnloadingLogic(...): thread ID %d\n", GetCurrentThreadId()));
		*unloadingLogicTakesOver = true;

		if (!unhookedAll) {
			logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...): haven't done any unhooking yet\n", logfile));

			graphics.onUnload();
			logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...): graphics.onUnload() called\n", logfile));

			std::vector<PVOID> thingsToNotDetach;
			thingsToNotDetach.push_back(hook_EndScene);
			if (ongoingPresentCall == GetCurrentThreadId()) {
				thingsToNotDetach.push_back(hook_Present);
			}

			detouring.detachAllButThese(thingsToNotDetach);
			unhookedAll = true;

			orig_EndSceneMutex.lock();
			*returnThisInTheCaller = orig_EndScene(device);
			orig_EndSceneMutex.unlock();

		} else {
			logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...): have already done unhooking. Unhooking self\n", logfile));
			// on this frame we've ensured the other hooks no longer run, assuming every hook runs in less than the duration of one frame
			detouring.detachAll();

			orig_EndSceneMutex.lock();
			*returnThisInTheCaller = orig_EndScene(device);
			orig_EndSceneMutex.unlock();

			if (ongoingPresentCall == GetCurrentThreadId()) {
				logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...): was called from a Present() call, so delegates reporting endSceneUnhooked to that\n", logfile));
				presentMustReportEndSceneUnhooked = true;
			} else {
				endSceneUnhookedMutex.lock();
				endSceneUnhooked = true;
				logwrap(fputs("EndScene::endSceneHookUnloadingLogic(...): endSceneUnhooked set to true\n", logfile));
				endSceneUnhookedConditionVariable.notify_all();
				endSceneUnhookedMutex.unlock();  // race condition against DLL unloading procedure starts on this line
			}

		}
	}
}
