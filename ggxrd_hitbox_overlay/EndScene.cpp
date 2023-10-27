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

EndScene endScene;

HRESULT __stdcall hook_EndScene(IDirect3DDevice9* device) {
	if (endScene.consumePresentFlag()) {
		bool needToClearHitDetection = false;
		if (*aswEngine == nullptr) {
			needToClearHitDetection = true;
		} else if (altModes.isGameInNormalMode(&needToClearHitDetection)) {
			endScene.endSceneHook(device);
		}
		if (needToClearHitDetection) {
			hitDetector.clearAllBoxes();
		}
	}
	return endScene.orig_EndScene(device);
}

HRESULT __stdcall hook_Present(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
	endScene.setPresentFlag();
	return endScene.orig_Present(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

bool EndScene::onDllMain() {
	orig_EndScene = (EndScene_t)direct3DVTable.getDirect3DVTable()[42];
	orig_Present = (Present_t)direct3DVTable.getDirect3DVTable()[17];

	if (!detouring.attach(&(PVOID&)(orig_EndScene),
		hook_EndScene,
		"EndScene")) return false;

	if (!detouring.attach(&(PVOID&)(orig_Present),
		hook_Present,
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
	entityList.onEndSceneStart();
	logOnce(fputs("entityList.onEndSceneStart() called\n", logfile));
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

		if (invisChipp.isCorrespondingChippInvis(ent)) continue;
		logOnce(fputs("invisChipp.isCorrespondingChippInvis(...) call successful\n", logfile));
		collectHitboxes(ent, active, &graphics.hurtboxes, &graphics.hitboxes, &graphics.points, &graphics.pushboxes);
		logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
		drawnEntities.push_back(ent);
		logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

		// Attached entities like dusts
		const auto attached = *(char**)(ent + 0x204);
		if (attached != nullptr) {
			logOnce(fprintf(logfile, "Attached entity: %p\n", attached));
			collectHitboxes(attached, active, &graphics.hurtboxes, &graphics.hitboxes, &graphics.points, &graphics.pushboxes);
			drawnEntities.push_back(attached);
		}
	}

	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawDetected();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));

	graphics.drawAll();
	logOnce(fputs("graphics.drawAll() call successful\n", logfile));

#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}
