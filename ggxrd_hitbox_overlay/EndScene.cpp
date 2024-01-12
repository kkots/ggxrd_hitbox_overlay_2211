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
#include "memoryFunctions.h"

EndScene endScene;

bool EndScene::onDllMain() {
	bool error = false;

	char** d3dvtbl = direct3DVTable.getDirect3DVTable();
	orig_EndScene = (EndScene_t)d3dvtbl[42];
	orig_Present = (Present_t)d3dvtbl[17];

	// there will actually be a deadlock during DLL unloading if we don't put Present first and EndScene second

	if (!detouring.attach(&(PVOID&)(orig_Present),
		hook_Present,
		&orig_PresentMutex,
		"Present")) return false;

	if (!detouring.attach(&(PVOID&)(orig_EndScene),
		hook_EndScene,
		&orig_EndSceneMutex,
		"EndScene")) return false;

	orig_SendUnrealPawnData = (SendUnrealPawnData_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x8b\x0d\x00\x00\x00\x00\x33\xdb\x53\xe8\x00\x00\x00\x00\xf3\x0f\x10\x80\x24\x04\x00\x00\xf3\x0f\x5c\x05\x00\x00\x00\x00\xf3\x0f\x10\x8e\xd0\x01\x00\x00\x0f\x2f\xc8\x76\x05\x8d\x43\x01\xeb\x02",
		"xx????xxxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxxxxx",
		{ -0x11 },
		&error, "SendUnrealPawnData");

	if (orig_SendUnrealPawnData) {
		void (HookHelp::*sendUnrealPawnDataHookPtr)(void) = &HookHelp::sendUnrealPawnDataHook;
		if (!detouring.attach(&(PVOID&)orig_SendUnrealPawnData,
			(PVOID&)sendUnrealPawnDataHookPtr,
			&orig_SendUnrealPawnDataMutex,
			"SendUnrealPawnData")) return false;
	}

	orig_ReadUnrealPawnData = (ReadUnrealPawnData_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"\x8b\xf1\x8b\x0e\xe8\x00\x00\x00\x00\x8b\x06\x8b\x48\x04\x89\x44\x24\x04\x85\xc9\x74\x1e\x83\x78\x08\x00\x74\x18\xf6\x81\x84\x00\x00\x00\x20",
		"xxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxx",
		{ -2 },
		&error, "ReadUnrealPawnData");

	if (orig_ReadUnrealPawnData) {
		void (HookHelp::*readUnrealPawnDataHookPtr)(void) = &HookHelp::readUnrealPawnDataHook;
		if (!detouring.attach(&(PVOID&)orig_ReadUnrealPawnData,
			(PVOID&)readUnrealPawnDataHookPtr,
			&orig_ReadUnrealPawnDataMutex,
			"ReadUnrealPawnData")) return false;
	}

	return !error;
}

bool EndScene::onDllDetach() {
	logwrap(fputs("EndScene::onDllDetach() called\n", logfile));
	if (*aswEngine) {
		entityList.populate();
		bool needToCallNoGravGifMode = gifMode.gifModeOn
			|| gifMode.gifModeToggleHideOpponentOnly
			|| gifMode.noGravityOn;
		gifMode.gifModeOn = false;
		gifMode.noGravityOn = false;
		gifMode.gifModeToggleHideOpponentOnly = false;
		if (needToCallNoGravGifMode) {
			noGravGifMode();
		}
	}
	return true;
}

void EndScene::HookHelp::sendUnrealPawnDataHook() {
	// this gets called many times every frame, presumably once per entity, but there're way more entities and they're not in the entityList.list
	++detouring.hooksCounter;
	detouring.markHookRunning("SendUnrealPawnData", true);
	endScene.sendUnrealPawnDataHook((char*)this);
	{
		bool needToUnlock = false;
		if (!endScene.orig_SendUnrealPawnDataMutexLocked || endScene.orig_SendUnrealPawnDataMutexThreadId != GetCurrentThreadId()) {
			needToUnlock = true;
			endScene.orig_SendUnrealPawnDataMutex.lock();
			endScene.orig_SendUnrealPawnDataMutexLocked = true;
			endScene.orig_SendUnrealPawnDataMutexThreadId = GetCurrentThreadId();
		}
		endScene.orig_SendUnrealPawnData((char*)this);
		if (needToUnlock) {
			endScene.orig_SendUnrealPawnDataMutexLocked = false;
			endScene.orig_SendUnrealPawnDataMutex.unlock();
		}
	}
	detouring.markHookRunning("SendUnrealPawnData", false);
	--detouring.hooksCounter;
}

void EndScene::HookHelp::readUnrealPawnDataHook() {
	// this read happens many times every frame and it appears to be synchronized with sendUnrealPawnDataHook via a simple SetEvent.
	// the model we built might not be super precise and probably is not how the game sends data over from the logic thread to the graphics thread,
	// but it's precise enough to never fail so we'll keep using it
	++detouring.hooksCounter;
	detouring.markHookRunning("ReadUnrealPawnData", true);
	endScene.readUnrealPawnDataHook((char*)this);
	detouring.markHookRunning("ReadUnrealPawnData", false);
	--detouring.hooksCounter;
}

void EndScene::sendUnrealPawnDataHook(char* thisArg) {
	if (*aswEngine == nullptr) return;
	entityList.populate();
	if (entityList.count < 1) return;
	if (*(char**)(*(char**)(entityList.slots[0] + 0x27a8) + 0x384) != thisArg) return;
	endScene.logic();
}

void EndScene::logic() {
	std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
	processKeyStrokes();
	if (graphics.drawDataPrepared.empty && !butDontPrepareBoxData) {
		bool oldNeedTakeScreenshot = graphics.drawDataPrepared.needTakeScreenshot;
		graphics.drawDataPrepared.clear();
		graphics.drawDataPrepared.needTakeScreenshot = oldNeedTakeScreenshot;

		bool needToClearHitDetection = false;
		if (gifMode.modDisabled) {
			needToClearHitDetection = true;
		}
		else if (*aswEngine == nullptr) {
			needToClearHitDetection = true;
			clearContinuousScreenshotMode();
		}
		else if (altModes.isGameInNormalMode(&needToClearHitDetection)) {
			if (!(game.isMatchRunning() ? true : altModes.roundendCameraFlybyType() != 8)) {
				needToClearHitDetection = true;
			}
			else {
				prepareDrawData(&needToClearHitDetection);
			}
		}
		if (needToClearHitDetection) {
			hitDetector.clearAllBoxes();
			throws.clearAllBoxes();
		}
		// Camera values are updated later, after this, in a updateCameraHook call
		graphics.drawDataPrepared.empty = false;
	}
}

void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("endSceneHook called\n", logfile));
	entityList.populate();
	logOnce(fputs("entityList.populate() called\n", logfile));
	if (!entityList.areAnimationsNormal()) {
		*needClearHitDetection = true;
#ifdef LOG_PATH
		didWriteOnce = true;
#endif
		return;
	}
	invisChipp.onEndSceneStart();
	drawnEntities.clear();

	noGravGifMode();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	bool frameHasChanged = false;
	unsigned int p1CurrentTimer = ~0;
	unsigned int p2CurrentTimer = ~0;
	if (entityList.count >= 1) {
		p1CurrentTimer = Entity{ entityList.slots[0] }.currentAnimDuration();
	}
	if (entityList.count >= 2) {
		p2CurrentTimer = Entity{ entityList.slots[1] }.currentAnimDuration();
	}
	if (p1CurrentTimer != p1PreviousTimeOfTakingScreen
		|| p2CurrentTimer != p2PreviousTimeOfTakingScreen) {
		frameHasChanged = true;
	}
	if (needContinuouslyTakeScreens) {
		if (frameHasChanged) {
			graphics.drawDataPrepared.needTakeScreenshot = true;
		}
		p1PreviousTimeOfTakingScreen = p1CurrentTimer;
		p2PreviousTimeOfTakingScreen = p2CurrentTimer;
	}
	else if (frameHasChanged) {
		p1PreviousTimeOfTakingScreen = ~0;
		p2PreviousTimeOfTakingScreen = ~0;
	}

	if (!gifMode.hitboxDisplayDisabled) {
		for (auto i = 0; i < entityList.count; i++)
		{
			Entity ent(entityList.list[i]);
			if (isEntityAlreadyDrawn(ent)) continue;

			bool active = ent.isActive();
			logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));

			if (invisChipp.needToHide(ent)) continue;
			DrawHitboxArrayCallParams hurtbox;
			collectHitboxes(ent, active, &hurtbox, &graphics.drawDataPrepared.hitboxes, &graphics.drawDataPrepared.points, &graphics.drawDataPrepared.pushboxes);
			HitDetector::WasHitInfo wasHitResult = hitDetector.wasThisHitPreviously(ent, hurtbox);
			if (!wasHitResult.wasHit) {
				graphics.drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
			}
			else {
				graphics.drawDataPrepared.hurtboxes.push_back({ true, hurtbox, wasHitResult.hurtbox });
			}
			logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
			drawnEntities.push_back(ent);
			logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

			// Attached entities like dusts
			const auto attached = *(char**)(ent + 0x204);
			if (attached != nullptr) {
				logOnce(fprintf(logfile, "Attached entity: %p\n", attached));
				collectHitboxes(attached, active, &hurtbox, &graphics.drawDataPrepared.hitboxes, &graphics.drawDataPrepared.points, &graphics.drawDataPrepared.pushboxes);
				graphics.drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
				drawnEntities.push_back(attached);
			}
		}

		logOnce(fputs("got past the entity loop\n", logfile));
		hitDetector.drawHits();
		logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
		throws.drawThrows();
		logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	}

#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}

void EndScene::readUnrealPawnDataHook(char* thisArg) {
	{
		std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
		if (!graphics.drawDataPrepared.empty && graphics.needNewDrawData) {
			graphics.drawDataUse.clear();
			graphics.drawDataPrepared.copyTo(&graphics.drawDataUse);
			graphics.drawDataPrepared.empty = true;
			graphics.needNewDrawData = false;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_ReadUnrealPawnDataMutex);
		orig_ReadUnrealPawnData(thisArg);
	}
}

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device) {
	++detouring.hooksCounter;
	detouring.markHookRunning("EndScene", true);
	if (endScene.consumePresentFlag()) {
		endScene.endSceneHook(device);
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
	{
		std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
		graphics.needNewDrawData = true;
	}
	{
		std::unique_lock<std::mutex> guard(camera.valuesPrepareMutex);
		graphics.needNewCameraData = true;
	}
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
	graphics.onEndSceneStart(device);
	drawOutlineCallParamsManager.onEndSceneStart();
	camera.onEndSceneStart();

	static bool everythingBroke = false;

	{
		std::unique_lock<std::mutex> guard(camera.valuesPrepareMutex);
		if (!camera.valuesPrepare.empty() && graphics.needNewCameraData) {
			if (camera.valuesPrepare.size() > 100) {
				everythingBroke = true;
				camera.valuesPrepare.clear();
			}
			if (!everythingBroke) {
				for (auto it = camera.valuesPrepare.begin(); it != camera.valuesPrepare.end(); ++it) {
					if (it->id == graphics.drawDataUse.id) {
						it->copyTo(camera.valuesUse);
						camera.valuesPrepare.erase(camera.valuesPrepare.begin(), it + 1);
						graphics.needNewCameraData = false;
						break;
					}
				}
			}
		}
	}

	{
		std::unique_lock<std::mutex> specialGuard(graphics.specialScreenshotFlagMutex);
		if (graphics.specialScreenshotFlag) {
			graphics.drawDataUse.needTakeScreenshot = true;
			graphics.specialScreenshotFlag = false;
		}
	}

	bool doYourThing = !gifMode.hitboxDisplayDisabled;

	if (!*aswEngine) {
		// since we store pointers to hitbox data instead of copies of it, when aswEngine disappears those are gone and we get a crash if we try to read them
		graphics.drawDataUse.clear();
	} else if (!altModes.isGameInNormalMode(nullptr)) {
		doYourThing = false;
	}

	if (doYourThing) {
		if (graphics.drawDataUse.needTakeScreenshot && !settings.dontUseScreenshotTransparency) {
			logwrap(fputs("Running the branch with if (needToTakeScreenshot)\n", logfile));
			graphics.takeScreenshotMain(device, false);
		}
		graphics.drawAll();
		if (graphics.drawDataUse.needTakeScreenshot && settings.dontUseScreenshotTransparency) {
			graphics.takeScreenshotMain(device, true);
		}

	} else if (graphics.drawDataUse.needTakeScreenshot) {
		graphics.takeScreenshotMain(device, true);
	}
	graphics.drawDataUse.needTakeScreenshot = false;
}

void EndScene::processKeyStrokes() {
	settings.readSettingsIfChanged();
	bool trainingMode = game.isTrainingMode();
	bool needToRunNoGravGifMode = false;
	keyboard.updateKeyStatuses();
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.gifModeToggle)) {
		// idk how atomic_bool reacts to ! and operator bool(), so we do it the arduous way
		if (gifMode.gifModeOn == true) {
			gifMode.gifModeOn = false;
			logwrap(fputs("GIF mode turned off\n", logfile));
			needToRunNoGravGifMode = true;
		} else if (trainingMode) {
			gifMode.gifModeOn = true;
			logwrap(fputs("GIF mode turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.gifModeToggleBackgroundOnly)) {
		if (gifMode.gifModeToggleBackgroundOnly == true) {
			gifMode.gifModeToggleBackgroundOnly = false;
			logwrap(fputs("GIF mode (darken background only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleBackgroundOnly = true;
			logwrap(fputs("GIF mode (darken background only) turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.gifModeToggleCameraCenterOnly)) {
		if (gifMode.gifModeToggleCameraCenterOnly == true) {
			gifMode.gifModeToggleCameraCenterOnly = false;
			logwrap(fputs("GIF mode (center camera only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleCameraCenterOnly = true;
			logwrap(fputs("GIF mode (center camera only) turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.gifModeToggleHideOpponentOnly)) {
		if (gifMode.gifModeToggleHideOpponentOnly == true) {
			gifMode.gifModeToggleHideOpponentOnly = false;
			logwrap(fputs("GIF mode (hide opponent only) turned off\n", logfile));
			needToRunNoGravGifMode = true;
		}
		else if (trainingMode) {
			gifMode.gifModeToggleHideOpponentOnly = true;
			logwrap(fputs("GIF mode (hide opponent only) turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.noGravityToggle)) {
		if (gifMode.noGravityOn == true) {
			gifMode.noGravityOn = false;
			logwrap(fputs("No gravity mode turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.noGravityOn = true;
			logwrap(fputs("No gravity mode turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.freezeGameToggle)) {
		if (freezeGame == true) {
			freezeGame = false;
			butDontPrepareBoxData = false;
			camera.butDontPrepareBoxData = false;
			logwrap(fputs("Freeze game turned off\n", logfile));
		}
		else if (trainingMode) {
			freezeGame = true;
			logwrap(fputs("Freeze game turned on\n", logfile));
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.slowmoGameToggle)) {
		if (game.slowmoGame == true) {
			game.slowmoGame = false;
			logwrap(fputs("Slowmo game turned off\n", logfile));
		}
		else if (trainingMode) {
			game.slowmoGame = true;
			logwrap(fputs("Slowmo game turned on\n", logfile));
		}
	}
	if (keyboard.gotPressed(settings.disableModKeyCombo)) {
		if (gifMode.modDisabled == true) {
			gifMode.modDisabled = false;
			logwrap(fputs("Mod enabled\n", logfile));
		} else {
			gifMode.modDisabled = true;
			logwrap(fputs("Mod disabled\n", logfile));
			needToRunNoGravGifMode = true;
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.disableHitboxDisplayToggle)) {
		if (gifMode.hitboxDisplayDisabled == true) {
			gifMode.hitboxDisplayDisabled = false;
			logwrap(fputs("Hitbox display enabled\n", logfile));
		} else {
			gifMode.hitboxDisplayDisabled = true;
			logwrap(fputs("Hitbox display disabled\n", logfile));
		}
	}
	bool allowNextFrameIsHeld = false;
	if (!gifMode.modDisabled) {
		allowNextFrameIsHeld = keyboard.isHeld(settings.allowNextFrameKeyCombo);
	}
	if (allowNextFrameIsHeld) {
		bool allowPress = false;
		if (allowNextFrameBeenHeldFor == 0) {
			allowPress = true;
		} else if (allowNextFrameBeenHeldFor >= 40) {
			allowNextFrameBeenHeldFor = 40;
			++allowNextFrameCounter;
			if (allowNextFrameCounter >= 10) {
				allowPress = true;
				allowNextFrameCounter = 0;
			}
		}
		if (trainingMode && allowPress) {
			game.allowNextFrame = true;
			logwrap(fputs("allowNextFrame set to true\n", logfile));
		}
		++allowNextFrameBeenHeldFor;
	} else {
		allowNextFrameBeenHeldFor = 0;
		allowNextFrameCounter = 0;
	}
	graphics.drawDataPrepared.needTakeScreenshot = false;
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.screenshotBtn)) {
		if (butDontPrepareBoxData) {
			std::unique_lock<std::mutex> specialGuard(graphics.specialScreenshotFlagMutex);
			graphics.specialScreenshotFlag = true;
		} else {
			graphics.drawDataPrepared.needTakeScreenshot = true;
		}
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.continuousScreenshotToggle)) {
		if (continuousScreenshotMode) {
			continuousScreenshotMode = false;
			logwrap(fputs("Continuous screenshot mode off\n", logfile));
		} else if (trainingMode) {
			continuousScreenshotMode = true;
			logwrap(fputs("Continuous screenshot mode on\n", logfile));
		}
	}
	bool screenshotPathEmpty = false;
	{
		std::unique_lock<std::mutex> guard(settings.screenshotPathMutex);
		screenshotPathEmpty = settings.screenshotPath.empty();
	}
	needContinuouslyTakeScreens = false;
	if (!gifMode.modDisabled
			&& (keyboard.isHeld(settings.screenshotBtn) && settings.allowContinuousScreenshotting || continuousScreenshotMode)
			&& *aswEngine
			&& trainingMode
			&& !screenshotPathEmpty) {
		needContinuouslyTakeScreens = true;
	}
	game.freezeGame = (allowNextFrameIsHeld || freezeGame) && trainingMode && !gifMode.modDisabled;
	if (!game.freezeGame) {
		butDontPrepareBoxData = false;
		camera.butDontPrepareBoxData = false;
	}
	if (!trainingMode || gifMode.modDisabled) {
		gifMode.gifModeOn = false;
		gifMode.noGravityOn = false;
		game.slowmoGame = false;
		gifMode.gifModeToggleBackgroundOnly = false;
		gifMode.gifModeToggleCameraCenterOnly = false;
		gifMode.gifModeToggleHideOpponentOnly = false;
		clearContinuousScreenshotMode();
	}
	if (needToRunNoGravGifMode) {
		if (*aswEngine) {
			entityList.populate();
			noGravGifMode();
		}
	}
}

void EndScene::noGravGifMode() {
	char playerIndex;
	char opponentIndex;
	playerIndex = game.getPlayerSide();
	if (playerIndex == 2) playerIndex = 0;
	opponentIndex = 1 - playerIndex;

	bool useGifMode = (gifMode.gifModeOn || gifMode.gifModeToggleHideOpponentOnly) && game.isTrainingMode();
	if (useGifMode) {
		for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
			it->wasFoundOnThisFrame = false;
		}
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent{entityList.list[i]};
			if (ent.team() != playerIndex) {
				const int currentScaleX = *(int*)(ent + 0x264);
				const int currentScaleY = *(int*)(ent + 0x268);
				const int currentScaleZ = *(int*)(ent + 0x26C);
				const int currentScaleDefault = *(int*)(ent + 0x2594);

				auto found = findHiddenEntity(ent);
				if (found == hiddenEntities.end()) {
					hiddenEntities.emplace_back();
					HiddenEntity& hiddenEntity = hiddenEntities.back();
					hiddenEntity.ent = ent;
					hiddenEntity.scaleX = currentScaleX;
					hiddenEntity.scaleY = currentScaleY;
					hiddenEntity.scaleZ = currentScaleZ;
					hiddenEntity.scaleDefault = currentScaleDefault;
					hiddenEntity.wasFoundOnThisFrame = true;
				} else {
					HiddenEntity& hiddenEntity = *found;
					if (currentScaleX != 0) {
						hiddenEntity.scaleX = currentScaleX;
					}
					if (currentScaleY != 0) {
						hiddenEntity.scaleY = currentScaleY;
					}
					if (currentScaleZ != 0) {
						hiddenEntity.scaleZ = currentScaleZ;
					}
					if (currentScaleDefault != 0) {
						hiddenEntity.scaleDefault = currentScaleDefault;
					}
					hiddenEntity.wasFoundOnThisFrame = true;
				}
				*(int*)(ent + 0x264) = 0;
				*(int*)(ent + 0x268) = 0;
				*(int*)(ent + 0x26C) = 0;
				*(int*)(ent + 0x2594) = 0;
			}
		}
		auto it = hiddenEntities.begin();
		while (it != hiddenEntities.end()) {
			if (!it->wasFoundOnThisFrame) {
				hiddenEntities.erase(it);
			}
			else {
				++it;
			}
		}
	} else {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent{ entityList.list[i] };
			auto found = findHiddenEntity(ent);
			if (found != hiddenEntities.end()) {
				const int currentScaleX = *(int*)(ent + 0x264);
				const int currentScaleY = *(int*)(ent + 0x268);
				const int currentScaleZ = *(int*)(ent + 0x26C);
				const int currentScaleDefault = *(int*)(ent + 0x2594);

				if (currentScaleX == 0) {
					*(int*)(ent + 0x264) = found->scaleX;
				}
				if (currentScaleY == 0) {
					*(int*)(ent + 0x268) = found->scaleY;
				}
				if (currentScaleZ == 0) {
					*(int*)(ent + 0x26C) = found->scaleZ;
				}
				if (currentScaleDefault == 0) {
					*(int*)(ent + 0x2594) = found->scaleDefault;
				}
			}
		}
		hiddenEntities.clear();
	}

	bool useNoGravMode = gifMode.noGravityOn && game.isTrainingMode();
	if (useNoGravMode) {
		*(int*)(entityList.slots[playerIndex] + 0x300) = 0;
	}
}

void EndScene::clearContinuousScreenshotMode() {
	continuousScreenshotMode = false;
	p1PreviousTimeOfTakingScreen = ~0;
	p2PreviousTimeOfTakingScreen = ~0;
}

std::vector<EndScene::HiddenEntity>::iterator EndScene::findHiddenEntity(const Entity& ent) {
	for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
		if (it->ent == ent) {
			return it;
		}
	}
	return hiddenEntities.end();
}

void EndScene::assignNextId(bool acquireLock) {
	std::unique_lock<std::mutex> guard;
	if (acquireLock) {
		guard = std::unique_lock<std::mutex>(graphics.drawDataPreparedMutex);
	}
	if (graphics.drawDataPrepared.id == 0xFFFFFFFF) {
		graphics.drawDataPrepared.id = 0;
	}
	else {
		++graphics.drawDataPrepared.id;
	}
	camera.nextId = graphics.drawDataPrepared.id;
}
