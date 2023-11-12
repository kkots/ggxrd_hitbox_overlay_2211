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
#include "Settings.h"
#include "Keyboard.h"
#include "GifMode.h"

EndScene endScene;

bool EndScene::onDllMain() {
	orig_EndScene = (EndScene_t)direct3DVTable.getDirect3DVTable()[42];
	orig_Present = (Present_t)direct3DVTable.getDirect3DVTable()[17];

	// there will actually be a deadlock during DLL unloading if we don't put Present first and EndScene second

	if (!detouring.attach(&(PVOID&)(orig_Present),
		hook_Present,
		&orig_PresentMutex,
		"Present")) return false;

	if (!detouring.attach(&(PVOID&)(orig_EndScene),
		hook_EndScene,
		&orig_EndSceneMutex,
		"EndScene")) return false;

	return true;
}

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device) {
	++detouring.hooksCounter;
	detouring.markHookRunning("EndScene", true);
	if (endScene.consumePresentFlag()) {

		endScene.processKeyStrokes();

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
	HRESULT result;
	{
		std::unique_lock<std::mutex> guard(endScene.orig_EndSceneMutex);
		result = endScene.orig_EndScene(device);
	}
	detouring.markHookRunning("EndScene", false);
	--detouring.hooksCounter;
	return result;
}

HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	++detouring.hooksCounter;
	detouring.markHookRunning("Present", true);
	HRESULT result = endScene.presentHook(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	detouring.markHookRunning("Present", false);
	--detouring.hooksCounter;
	return result;
}

HRESULT EndScene::presentHook(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	setPresentFlag();
	std::unique_lock<std::mutex> guard(endScene.orig_PresentMutex);
	HRESULT result = orig_Present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);  // may call d3d9.dll::EndScene() (and, consecutively, the hook)
	return result;
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

	noGravGifMode();

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
	if (*aswEngine) {
		entityList.populate();
		gifMode.gifModeOn = false;
		gifMode.noGravityOn = false;
		noGravGifMode();
	}
	return true;
}

void EndScene::processKeyStrokes() {
	keyboard.updateKeyStatuses();
	if (keyboard.gotPressed(settings.gifModeToggle)) {
		// idk how atomic_bool reacts to ! and operator bool(), so we do it the arduous way
		if (gifMode.gifModeOn == true) {
			gifMode.gifModeOn = false;
			logwrap(fputs("GIF mode turned off\n", logfile));
		} else {
			gifMode.gifModeOn = true;
			logwrap(fputs("GIF mode turned on\n", logfile));
		}
	}
	if (keyboard.gotPressed(settings.noGravityToggle)) {
		if (gifMode.noGravityOn == true) {
			gifMode.noGravityOn = false;
			logwrap(fputs("No gravity mode turned off\n", logfile));
		}
		else {
			gifMode.noGravityOn = true;
			logwrap(fputs("No gravity mode turned on\n", logfile));
		}
	}
	if (!game.isTrainingMode()) {
		gifMode.gifModeOn = false;
		gifMode.noGravityOn = false;
	}
}

void EndScene::noGravGifMode() {
	char playerIndex;
	char opponentIndex;
	playerIndex = game.getPlayerSide();
	if (playerIndex == 2) playerIndex = 0;
	opponentIndex = 1 - playerIndex;

	bool useGifMode = gifMode.gifModeOn && game.isTrainingMode();
	if (scaleIs0 && !useGifMode) {
		if (entityList.count > opponentIndex) {
			*(int*)(entityList.slots[opponentIndex] + 0x264) = 1000;
			*(int*)(entityList.slots[opponentIndex] + 0x268) = 1000;
			*(int*)(entityList.slots[opponentIndex] + 0x26C) = 1000;

			*(int*)(entityList.slots[opponentIndex] + 0x2594) = 1000;
		}
		scaleIs0 = false;
	}
	if (useGifMode) {
		if (entityList.count > opponentIndex) {
			*(int*)(entityList.slots[opponentIndex] + 0x264) = 0;
			*(int*)(entityList.slots[opponentIndex] + 0x268) = 0;
			*(int*)(entityList.slots[opponentIndex] + 0x26C) = 0;

			*(int*)(entityList.slots[opponentIndex] + 0x2594) = 0;
		}
		scaleIs0 = true;
	}

	bool useNoGravMode = gifMode.noGravityOn && game.isTrainingMode();
	if (useNoGravMode) {
		*(int*)(entityList.slots[playerIndex] + 0x300) = 0;
	}
}
