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
#include "WinError.h"
#include "UI.h"
#include "CustomWindowMessages.h"
#include "Hud.h"

EndScene endScene;
PlayerInfo emptyPlayer {0};
ProjectileInfo emptyProjectile;

bool EndScene::onDllMain() {
	bool error = false;
	
	orig_WndProc = (WNDPROC)sigscanOffset(
		"GuiltyGearXrd.exe",
		"81 fb 18 02 00 00 75 0f 83 fd 01 77 28 5d b8 44 51 4d 42 5b c2 10 00",
		{ -0xA },
		&error, "WndProc");
	// why not use orig_WndProc = (WNDPROC)SetWindowLongPtrW(keyboard.thisProcessWindow, GWLP_WNDPROC, (LONG_PTR)hook_WndProc);?
	// because if you use the patcher, it will load the DLL on startup, the DLL will swap out the WndProc,
	// but then the game swaps out the WndProc with its own afterwards. So clearly, we need to load the DLL
	// a bit later, and that needs to be fixed in the patcher, but a quick solution is to hook the
	// WndProc directly.

	if (orig_WndProc) {
		if (!detouring.attach(&(PVOID&)orig_WndProc,
			hook_WndProc,
			&orig_WndProcMutex,
			"WndProc")) return false;
	}
	
	uintptr_t TrainingEtc_OneDamage = sigscanStrOffset(
		"GuiltyGearXrd.exe:.rdata",
		"TrainingEtc_OneDamage",
		&error, "TrainingEtc_OneDamage");
	if (!error) {
		std::vector<char> sig;//[9] { "\xc7\x40\x28\x00\x00\x00\x00\xe8" };
		std::vector<char> mask;
		byteSpecificationToSigMask("c7 40 28 ?? ?? ?? ?? e8", sig, mask);
		substituteWildcard(sig.data(), mask.data(), 0, (void*)TrainingEtc_OneDamage);
		
		uintptr_t drawTextWithIconsCall = sigscanBufOffset(
			"GuiltyGearXrd.exe",
			sig.data(),
			sig.size() - 1,
			{ 7 },
			&error, "drawTextWithIconsCall");
		if (drawTextWithIconsCall) {
			drawTextWithIcons = (drawTextWithIcons_t)followRelativeCall(drawTextWithIconsCall);
			logwrap(fprintf(logfile, "drawTextWithIcons: %p\n", drawTextWithIcons));
			orig_drawTrainingHud = game.trainingHudTick;
			logwrap(fprintf(logfile, "orig_drawTrainingHud final location: %p\n", (void*)orig_drawTrainingHud));
		}
		if (orig_drawTrainingHud) {
			void (HookHelp::*drawTrainingHudHookPtr)(void) = &HookHelp::drawTrainingHudHook;
			if (!detouring.attach(&(PVOID&)orig_drawTrainingHud,
				(PVOID&)drawTrainingHudHookPtr,
				&orig_drawTrainingHudMutex,
				"drawTrainingHud")) return false;
		}
		if (!drawTextWithIcons) {
			error = true;
		}
	}
	
	orig_USkeletalMeshComponent_UpdateTransform = (USkeletalMeshComponent_UpdateTransform_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 0d ?? ?? ?? ?? 33 db 53 e8 ?? ?? ?? ?? f3 0f 10 80 24 04 00 00 f3 0f 5c 05 ?? ?? ?? ?? f3 0f 10 8e d0 01 00 00 0f 2f c8 76 05 8d 43 01 eb 02",
		{ -0x11 },
		&error, "USkeletalMeshComponent_UpdateTransform");

	if (orig_USkeletalMeshComponent_UpdateTransform) {
		void (HookHelp::*USkeletalMeshComponent_UpdateTransformHookPtr)(void) = &HookHelp::USkeletalMeshComponent_UpdateTransformHook;
		if (!detouring.attach(&(PVOID&)orig_USkeletalMeshComponent_UpdateTransform,
			(PVOID&)USkeletalMeshComponent_UpdateTransformHookPtr,
			&orig_USkeletalMeshComponent_UpdateTransformMutex,
			"USkeletalMeshComponent_UpdateTransform")) return false;
	}
	
	// FUpdatePrimitiveTransformCommand::Apply()
	orig_FUpdatePrimitiveTransformCommand_Apply = (FUpdatePrimitiveTransformCommand_Apply_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b f1 8b 0e e8 ?? ?? ?? ?? 8b 06 8b 48 04 89 44 24 04 85 c9 74 1e 83 78 08 00 74 18 f6 81 84 00 00 00 20",
		{ -2 },
		&error, "FUpdatePrimitiveTransformCommand::Apply");

	if (orig_FUpdatePrimitiveTransformCommand_Apply) {
		void (HookHelp::*FUpdatePrimitiveTransformCommand_ApplyHookPtr)(void) = &HookHelp::FUpdatePrimitiveTransformCommand_ApplyHook;
		if (!detouring.attach(&(PVOID&)orig_FUpdatePrimitiveTransformCommand_Apply,
			(PVOID&)FUpdatePrimitiveTransformCommand_ApplyHookPtr,
			&orig_FUpdatePrimitiveTransformCommand_ApplyMutex,
			"FUpdatePrimitiveTransformCommand::Apply")) return false;
	}
	
	superflashInstigatorOffset = sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 86 ?? ?? ?? ?? 3b c3 74 09 ff 48 24 89 9e ?? ?? ?? ?? 89 be ?? ?? ?? ?? ff 47 24 8b 8f bc 01 00 00 49 89 8e ?? ?? ?? ?? 8b 97 c0 01 00 00 4a",
		{ 37, 0 },
		NULL, "superflashInstigatorOffset");
	if (superflashInstigatorOffset) {
		superflashCounterAllOffset = superflashInstigatorOffset + 4;
		superflashCounterSelfOffset = superflashInstigatorOffset + 8;
	}
	
	orig_endSceneCaller = *(endSceneCaller_t*)(**(void****)direct3DVTable.d3dManager + 0x4b);
	void (HookHelp::*endSceneCallerHookPtr)(int param1, int param2, int param3) = &HookHelp::endSceneCallerHook;
	if (!detouring.attach(&(PVOID&)orig_endSceneCaller,
		(PVOID&)endSceneCallerHookPtr,
		&orig_endSceneCallerMutex,
		"endSceneCaller")) return false;
	
	shutdownFinishedEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
	if (!shutdownFinishedEvent || shutdownFinishedEvent == INVALID_HANDLE_VALUE) {
		WinError winErr;
		logwrap(fprintf(logfile, "Failed to create event: %ls\n", winErr.getMessage()));
		error = true;
	}
	
	uintptr_t bbscrJumptable = sigscanOffset(
		"GuiltyGearXrd.exe",
		"83 ?? 04 85 ?? 74 07 83 ?? 28 74 02 8b ?? 81 ?? 88 09 00 00 0f 87 ?? ?? ?? ?? ff 24",
		{ 29, 0 },
		&error, "bbscrJumptable");
	uintptr_t bbscrCreateObjectWithArgCallPlace = 0;
	uintptr_t bbscrCreateObjectWithArgCall = 0;
	uintptr_t bbscrCreateParticleWithArgCallPlace = 0;
	uintptr_t bbscrCreateParticleWithArgCall = 0;
	uintptr_t bbscrRunOnObjectCallPlace = 0;
	uintptr_t bbscrRunOnObjectCall = 0;
	if (bbscrJumptable) {
		bbscrCreateObjectWithArgCallPlace = *(uintptr_t*)(bbscrJumptable + 445*4);
		bbscrCreateParticleWithArgCallPlace = *(uintptr_t*)(bbscrJumptable + 449*4);
		bbscrRunOnObjectCallPlace = *(uintptr_t*)(bbscrJumptable + 41*4);
	}
	if (bbscrCreateObjectWithArgCallPlace) {
		bbscrCreateObjectWithArgCall = sigscanForward(bbscrCreateObjectWithArgCallPlace, "e8");
	}
	if (bbscrCreateParticleWithArgCallPlace) {
		bbscrCreateParticleWithArgCall = sigscanForward(bbscrCreateParticleWithArgCallPlace, "e8");
	}
	if (bbscrRunOnObjectCallPlace) {
		bbscrRunOnObjectCall = sigscanForward(bbscrRunOnObjectCallPlace, "e8");
	}
	if (bbscrCreateObjectWithArgCall) {
		orig_BBScr_createObjectWithArg = (BBScr_createObjectWithArg_t)followRelativeCall(bbscrCreateObjectWithArgCall);
	}
	if (bbscrCreateParticleWithArgCall) {
		orig_BBScr_createParticleWithArg = (BBScr_createParticleWithArg_t)followRelativeCall(bbscrCreateParticleWithArgCall);
	}
	if (bbscrRunOnObjectCall) {
		orig_BBScr_runOnObject = (BBScr_runOnObject_t)followRelativeCall(bbscrRunOnObjectCall);
	}
	
	if (orig_BBScr_createObjectWithArg) {
		void (HookHelp::*BBScr_createObjectWithArgHookPtr)(const char*, unsigned int) = &HookHelp::BBScr_createObjectWithArgHook;
		if (!detouring.attach(&(PVOID&)orig_BBScr_createObjectWithArg,
			(PVOID&)BBScr_createObjectWithArgHookPtr,
			&orig_BBScr_createObjectWithArgMutex,
			"BBScr_createObjectWithArg")) return false;
	}
	if (orig_BBScr_createParticleWithArg) {
		void (HookHelp::*BBScr_createParticleWithArgHookPtr)(const char*, unsigned int) = &HookHelp::BBScr_createParticleWithArgHook;
		if (!detouring.attach(&(PVOID&)orig_BBScr_createParticleWithArg,
			(PVOID&)BBScr_createParticleWithArgHookPtr,
			&orig_BBScr_createParticleWithArgMutex,
			"BBScr_createParticleWithArg")) return false;
	}
	if (orig_BBScr_runOnObject) {
		void (HookHelp::*BBScr_runOnObjectHookPtr)(int) = &HookHelp::BBScr_runOnObjectHook;
		if (!detouring.attach(&(PVOID&)orig_BBScr_runOnObject,
			(PVOID&)BBScr_runOnObjectHookPtr,
			&orig_BBScr_runOnObjectMutex,
			"BBScr_runOnObject")) return false;
	}
	std::vector<char> sig;
	std::vector<char> mask;
	byteSpecificationToSigMask("c7 ?? ?? ?? ?? ?? 66 0f ef c0 8d ?? ?? ?? ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? ?? 89 ?? ?? ?? 66 0f d6 ?? ?? ?? ?? ?? e8",
		sig, mask);
	uintptr_t PawnVtable = sigscanOffset(
		"GuiltyGearXrd.exe",
		sig.data(),
		mask.data(),
		{ 2, 0 },
		&error, "PawnVtable");
	if (PawnVtable) {
		orig_setAnim = *(setAnim_t*)(PawnVtable + 0x44);
		orig_pawnInitialize = *(pawnInitialize_t*)(PawnVtable);
	}
	
	if (orig_setAnim) {
		void (HookHelp::*setAnimHookPtr)(const char*) = &HookHelp::setAnimHook;
		if (!detouring.attach(&(PVOID&)orig_setAnim,
			(PVOID&)setAnimHookPtr,
			&orig_setAnimMutex,
			"setAnim")) return false;
	}
	
	if (orig_pawnInitialize) {
		void (HookHelp::*pawnInitializeHookPtr)(void*) = &HookHelp::pawnInitializeHook;
		if (!detouring.attach(&(PVOID&)orig_pawnInitialize,
			(PVOID&)pawnInitializeHookPtr,
			&orig_pawnInitializeMutex,
			"pawnInitialize")) return false;
	}
	
	uintptr_t logicOnFrameAfterHitPiece = sigscanOffset(
		"GuiltyGearXrd.exe",
		"77 10 89 ?? b4 01 00 00 c7 ?? b8 01 00 00 02 00 00 00",
		&error, "logicOnFrameAfterHitPiece");
	if (logicOnFrameAfterHitPiece) {
		orig_logicOnFrameAfterHit = (logicOnFrameAfterHit_t)scrollUpToBytes((char*)logicOnFrameAfterHitPiece,
			"\x83\xec\x14", 3);
	}
	if (orig_logicOnFrameAfterHit) {
		void (HookHelp::*logicOnFrameAfterHitHookPtr)(int, int) = &HookHelp::logicOnFrameAfterHitHook;
		if (!detouring.attach(&(PVOID&)orig_logicOnFrameAfterHit,
			(PVOID&)logicOnFrameAfterHitHookPtr,
			&orig_logicOnFrameAfterHitMutex,
			"logicOnFrameAfterHit")) return false;
	}

	return !error;
}

bool EndScene::onDllDetach() {
	logwrap(fputs("EndScene::onDllDetach() called\n", logfile));
	shutdown = true;
	if (!logicThreadId || logicThreadId == detouring.dllMainThreadId) return true;
	HANDLE logicThreadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, logicThreadId);
	if (!logicThreadHandle) {
		WinError winErr;
		logwrap(fprintf(logfile, "EndScene failed to open logic thread handle: %ls\n", winErr.getMessage()));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return false;
	}
	if (GetProcessIdOfThread(logicThreadHandle) != GetCurrentProcessId()) {
		CloseHandle(logicThreadHandle);
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive"));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return true;
	}
	DWORD exitCode;
	bool stillActive = GetExitCodeThread(logicThreadHandle, &exitCode) && exitCode == STILL_ACTIVE;
	CloseHandle(logicThreadHandle);
	
	if (!stillActive) {
		logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because thread is no longer alive (2)"));
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		return true;
	}
	
	logwrap(fputs("EndScene calling WaitForSingleObject\n", logfile));
	if (WaitForSingleObject(shutdownFinishedEvent, 300) == WAIT_OBJECT_0) {
		logwrap(fprintf(logfile, "EndScene freed resources successfully\n"));
		return true;
	}
	logwrap(fprintf(logfile, "EndScene freeing resources on DLL thread, because WaitForSingleObject did not return success"));
	hud.onDllDetach();
	ui.onDllDetachNonGraphics();
	return true;
}

void EndScene::HookHelp::USkeletalMeshComponent_UpdateTransformHook() {
	// this gets called many times every frame, presumably once per entity, but there're way more entities and they're not in the entityList.list
	HookGuard hookGuard("USkeletalMeshComponent_UpdateTransform");
	if (!endScene.shutdown && !graphics.shutdown) {
		endScene.USkeletalMeshComponent_UpdateTransformHook((char*)this);
	}
	{
		bool needToUnlock = false;
		if (!endScene.orig_USkeletalMeshComponent_UpdateTransformMutexLocked || endScene.orig_USkeletalMeshComponent_UpdateTransformMutexThreadId != GetCurrentThreadId()) {
			needToUnlock = true;
			endScene.orig_USkeletalMeshComponent_UpdateTransformMutex.lock();
			endScene.orig_USkeletalMeshComponent_UpdateTransformMutexLocked = true;
			endScene.orig_USkeletalMeshComponent_UpdateTransformMutexThreadId = GetCurrentThreadId();
		}
		endScene.orig_USkeletalMeshComponent_UpdateTransform((char*)this);  // this method likes to call itself
		if (needToUnlock) {
			endScene.orig_USkeletalMeshComponent_UpdateTransformMutexLocked = false;
			endScene.orig_USkeletalMeshComponent_UpdateTransformMutex.unlock();
		}
	}
}

void EndScene::HookHelp::FUpdatePrimitiveTransformCommand_ApplyHook() {
	// this read happens many times every frame and it appears to be synchronized with USkeletalMeshComponent_UpdateTransformHook via a simple SetEvent.
	// the model we built might not be super precise and probably is not how the game sends data over from the logic thread to the graphics thread,
	// but it's precise enough to never fail so we'll keep using it
	HookGuard hookGuard("FUpdatePrimitiveTransformCommand::Apply");
	endScene.FUpdatePrimitiveTransformCommand_ApplyHook((char*)this);
}

void EndScene::HookHelp::drawTrainingHudHook() {
	HookGuard hookGuard("drawTrainingHud");
	endScene.drawTrainingHudHook((char*)this);
}

void EndScene::USkeletalMeshComponent_UpdateTransformHook(char* thisArg) {
	if (*aswEngine == nullptr) return;
	entityList.populate();
	if (entityList.count < 1) return;
	// the thing at 0x27a8 is a REDPawn_Player, and the thing at 0x384 is a SkeletalMeshComponent
	if (*(char**)(*(char**)(entityList.slots[0] + 0x27a8) + 0x384) != thisArg) return;
	endScene.logic();
}

void EndScene::logic() {
	std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
	actUponKeyStrokesThatAlreadyHappened();
	if (graphics.drawDataPrepared.empty && !butDontPrepareBoxData) {
		bool oldNeedTakeScreenshot = graphics.drawDataPrepared.needTakeScreenshot;
		graphics.drawDataPrepared.clear();
		graphics.drawDataPrepared.needTakeScreenshot = oldNeedTakeScreenshot;

		bool needToClearHitDetection = false;
		if (gifMode.modDisabled) {
			needToClearHitDetection = true;
		}
		else {
			bool isPauseMenu;
			bool isNormalMode = altModes.isGameInNormalMode(&needToClearHitDetection, &isPauseMenu);
			bool isRunning = game.isMatchRunning() || altModes.roundendCameraFlybyType() != 8;
			entityList.populate();
			bool areAnimationsNormal = entityList.areAnimationsNormal();
			if (isNormalMode) {
				if (!isRunning || !areAnimationsNormal) {
					needToClearHitDetection = true;
				}
				else {
					prepareDrawData(&needToClearHitDetection);
				}
			}
		}
		if (needToClearHitDetection) {
			hitDetector.clearAllBoxes();
			throws.clearAllBoxes();
		}
		// Camera values are updated later, after this, in a updateCameraHook call
		graphics.drawDataPrepared.empty = false;
	}
	if (!drawDataPrepared) {
		ui.drawData = nullptr;
		ui.needInitFont = false;
		if (!ui.shutdownGraphics) {
			ui.prepareDrawData();
		}
		drawDataPrepared = true;
	}
}

void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("endSceneHook called\n", logfile));
	invisChipp.onEndSceneStart();
	drawnEntities.clear();

	noGravGifMode();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	DWORD aswEngineTickCount = *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
	bool frameHasChangedForScreenshot = previousTimeOfTakingScreen != aswEngineTickCount;
	if (needContinuouslyTakeScreens) {
		if (frameHasChangedForScreenshot) {
			graphics.drawDataPrepared.needTakeScreenshot = true;
		}
		previousTimeOfTakingScreen = aswEngineTickCount;
	}
	else if (frameHasChangedForScreenshot) {
		previousTimeOfTakingScreen = ~0;
	}
	bool frameHasChanged = prevAswEngineTickCount != aswEngineTickCount;
	prevAswEngineTickCount = aswEngineTickCount;
	
	if (frameHasChanged) {
		bool needCatchEntities = false;
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			if (!player.pawn) {
				player.index = i;
				initializePawn(player, entityList.slots[i]);
				needCatchEntities = true;
			}
		}
		if (needCatchEntities && projectiles.empty()) {
			// probably the mod was loaded in mid-game
			for (int i = 2; i < entityList.count; ++i) {
				onObjectCreated(entityList.slots[i], entityList.list[i], entityList.list[i].animationName());
			}
		}
		
		for (auto it = projectiles.begin(); it != projectiles.end();) {
			bool found = false;
			if (it->ptr) {
				for (int i = 0; i < entityList.count; ++i) {
					Entity ent = entityList.list[i];
					if (it->ptr == ent) {
						found = true;
						break;
					}
				}
			}
			if (!found) {
				if (it->landedHit) {
					it->ptr = nullptr;
					it->markActive = true;
					++it;
					continue;
				}
				it = projectiles.erase(it);
			} else {
				it->fill(it->ptr);
				it->markActive = it->landedHit;
				++it;
			}
		}
		
		int distanceBetweenPlayers = entityList.slots[0].posX() - entityList.slots[1].posX();
		if (distanceBetweenPlayers < 0) distanceBetweenPlayers = -distanceBetweenPlayers;
		
		bool comboStarted = false;
		for (int i = 0; i < 2; ++i) {
			Entity ent = entityList.slots[i];
			PlayerInfo& player = players[i];
			if (ent.inPain() && !player.inPain) {
				comboStarted = true;
				break;
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			Entity ent = player.pawn;
			Entity otherEnt = other.pawn;
			player.hp = ent.hp();
			player.gutsRating = ent.gutsRating();
			player.gutsPercentage = ent.calculateGuts();
			player.risc = ent.risc();
			int tension = ent.tension();
			int tensionGain = tension - player.tension;
			player.tension = tension;
			int burst = game.getBurst(i);
			int burstGain = burst - player.burst;
			player.burst = burst;
			player.tensionPulse = ent.tensionPulse();
			player.negativePenaltyTimer = ent.negativePenaltyTimer();
			player.negativePenalty = ent.negativePenalty();
			player.tensionPulsePenalty = ent.tensionPulsePenalty();
			player.cornerPenalty = ent.cornerPenalty();
			entityManager.calculateTensionPulsePenaltyGainModifier(
				distanceBetweenPlayers,
				player.tensionPulse,
				&player.tensionPulsePenaltyGainModifier_distanceModifier,
				&player.tensionPulsePenaltyGainModifier_tensionPulseModifier);
			entityManager.calculateTensionGainModifier(
				distanceBetweenPlayers,
				player.negativePenaltyTimer,
				player.tensionPulse,
				&player.tensionGainModifier_distanceModifier,
				&player.tensionGainModifier_negativePenaltyModifier,
				&player.tensionGainModifier_tensionPulseModifier);
			player.extraTensionGainModifier = entityManager.calculateExtraTensionGainModifier(ent);
			if (comboStarted) {
				if (tensionGainOnLastHitUpdated[i]) {
					player.tensionGainLastCombo = tensionGainOnLastHit[i];
					tensionGain = player.tension - tensionRecordedHit[i];
				} else {
					player.tensionGainLastCombo = 0;
				}
				if (burstGainOnLastHitUpdated[i]) {
					player.burstGainLastCombo = burstGainOnLastHit[i];
					burstGain = player.burst - burstRecordedHit[i];
				} else {
					player.burstGainLastCombo = 0;
				}
			}
			player.inPain = ent.inPain();
			if (player.inPain || otherEnt.inPain()) {
				player.tensionGainLastCombo += tensionGain;
				player.burstGainLastCombo += burstGain;
			}
			if (player.tensionGainLastCombo > player.tensionGainMaxCombo) {
				player.tensionGainMaxCombo = player.tensionGainLastCombo;
			}
			if (player.burstGainLastCombo > player.burstGainMaxCombo) {
				player.burstGainMaxCombo = player.burstGainLastCombo;
			}
			player.receivedComboCountTensionGainModifier = entityManager.calculateReceivedComboCountTensionGainModifier(
				player.inPain,
				ent.comboCount());
			player.dealtComboCountTensionGainModifier = entityManager.calculateDealtComboCountTensionGainModifier(
				otherEnt.inPain(),
				otherEnt.comboCount());
			player.tensionPulsePenaltySeverity = entityManager.calculateTensionPulsePenaltySeverity(player.tensionPulsePenalty);
			player.cornerPenaltySeverity = entityManager.calculateCornerPenaltySeverity(player.cornerPenalty);
			if (tensionGainOnLastHitUpdated[i]) {
				player.tensionGainOnLastHit = tensionGainOnLastHit[i];
			}
			if (burstGainOnLastHitUpdated[i]) {
				player.burstGainOnLastHit = burstGainOnLastHit[i];
			}
			tensionGainOnLastHitUpdated[i] = false;
			burstGainOnLastHitUpdated[i] = false;
	
			player.stun = ent.stun();
			player.stunThreshold = ent.stunThreshold();
			player.blockstun = ent.blockstun();
			player.hitstun = ent.hitstun();
			int hitstop = ent.hitstop();
			if (ent.hitSomethingOnThisFrame()) {
				player.hitstop = 0;
				player.hitstopMax = hitstop - 1;
				player.setHitstopMax = true;
			} else {
				player.hitstop = hitstop;
			}
			player.airborne = ent.y() > 0;  // there's also tumbling state in which you're airborne, check *(DWORD*)(end + 0x4d40) & 0x4 != 0 if you need it. This flag is set on any airborne type of hitstun
			player.wakeupTiming = 0;
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			player.cmnActIndex = cmnActIndex;
			if (cmnActIndex == CmnActFDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceDown;
			} else if (cmnActIndex == CmnActBDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceUp;
			} else if (cmnActIndex == CmnActWallHaritsukiGetUp) {
				player.wakeupTiming = 15;
			} else if (cmnActIndex == CmnActUkemi) {
				player.wakeupTiming = 9;
			}
			player.onTheDefensive = player.blockstun
				|| player.inPain
				|| cmnActIndex == CmnActUkemi
				|| ent.gettingUp();
			player.idleNext = ent.isIdle();
			player.isLanding = cmnActIndex == CmnActJumpLanding || cmnActIndex == CmnActLandingStiff;
			player.isLandingOrPreJump = player.isLanding || cmnActIndex == CmnActJumpPre;
			if (!player.isLandingOrPreJump) {
				if (player.landingOrPreJumpFrames) {
					if (!player.idleNext) {
						player.idlePlus = false;
						player.timePassed = 0;
						player.timePassedLanding = 0;
					}
					player.landingOrPreJumpFrames = 0;
				}
				player.landingOrPreJump = false;
			}
			if (cmnActIndex == CmnActJumpPre) {
				player.frameAdvantageValid = false;
				other.frameAdvantageValid = false;
			}
			player.animFrame = ent.currentAnimDuration();
			player.hitboxesCount = 0;
		}
		// I need another loop because in the next one each player refers to other's timePassed and I modified it
	
		for (int i = 0; i < 2; i++) {
			Entity ent = entityList.slots[i];
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			if (player.idleNext != player.idle || player.wasIdle && !player.idleNext || player.setBlockstunMax && !player.idleNext) {
				player.idle = player.idleNext;
				if (!player.idle) {
					player.landingOrPreJump = player.isLandingOrPreJump;
					if (player.onTheDefensive) {
						player.startedDefending = true;
						if (player.cmnActIndex != CmnActUkemi) {
							player.hitstopMax = player.hitstop;
							player.setHitstopMax = true;
							if (player.setBlockstunMax) {
								--player.blockstunMax;
							}
						}
						
						if (other.timeSinceLastGap > 45) {
							other.clearGaps();
						} else {
							other.addGap(other.timeSinceLastGap);
						}
						other.timeSinceLastGap = 0;
					}
				} else if (player.airborne) {
					player.needLand = true;
					player.landingFrameAdvantageValid = false;
					other.landingFrameAdvantageValid = false;
				}
			}
			player.gettingUp = ent.gettingUp();
			if (player.needLand
					&& (player.isLanding
					|| player.onTheDefensive && !player.airborne)) {
				player.needLand = false;
			}
			bool idlePlus = player.idle || player.landingOrPreJump;
			if (idlePlus != player.idlePlus) {
				player.timePassed = 0;
				player.idlePlus = idlePlus;
			}
			const char* animName = ent.animationName();
			// the wasIdle && !player.idle is for when you link a move into that same move. The animation doesn't change but you started/restarted the move
			if (strcmp(player.anim, animName) != 0 || player.wasIdle && !player.idle) {
				memcpy(player.anim, animName, 32);
				player.moveOriginatedInTheAir = false;
				if (!player.isLanding) {
					if (!player.idlePlus && !(!player.wasIdle && player.cmnActIndex == CmnActRomanCancel)) {
						if (player.setHitstunMax || player.setBlockstunMax) {
							player.displayHitstop = false;
						}
						if (!player.onTheDefensive) {
							player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
						}
						player.moveOriginatedInTheAir = player.airborne;
						
						player.startupProj = 0;
						player.activesProj.clear();
						
						player.startup = 0;
						player.startedUp = false;
						player.actives.clear();
						player.recovery = 0;
						player.total = 0;
						
						player.landingRecovery = 0;
						player.superfreezeStartup = 0;
						if (player.cmnActIndex != CmnActRomanCancel) {
							for (ProjectileInfo& projectile : projectiles) {
								if (projectile.team == player.index) {
									projectile.disabled = true;
									projectile.startedUp = false;
									projectile.actives.clear();
								}
							}
						}
					}
				}
			}
		}
		
		// This is a separate loop because it depends on another player's idlePlus, which I changed in the previous loop
		for (PlayerInfo& player : players) {
			PlayerInfo& other = players[1 - player.index];
			if (player.idleLanding != (player.idle || player.wasIdle)) {
				if (!(!player.idleLanding && player.needLand)) {  // we can't change idleLanding from false to true while needLand is true
					player.idleLanding = (player.idle || player.wasIdle);
					if (player.idleLanding && player.isLanding) {
						if (other.idlePlus) {
							measuringLandingFrameAdvantage = -1;
							if (other.timePassed > player.timePassedLanding) {
								player.landingFrameAdvantage = -player.timePassedLanding;
							} else {
								player.landingFrameAdvantage = -other.timePassed;
							}
							other.landingFrameAdvantage = -player.landingFrameAdvantage;
							if (-player.landingFrameAdvantage != player.landingOrPreJumpFrames) {
								player.landingFrameAdvantageValid = true;
								other.landingFrameAdvantageValid = true;
							}
						} else if (measuringLandingFrameAdvantage == -1) {
							measuringLandingFrameAdvantage = player.index;
							player.landingFrameAdvantageValid = false;
							other.landingFrameAdvantageValid = false;
							player.landingFrameAdvantage = 0;
							other.landingFrameAdvantage = 0;
						}
					}
					player.timePassedLanding = 0;
				}
			}
		}
		
		// This is a separate loop because it depends on another player's timePassedLanding, which I changed in the previous loop
		for (PlayerInfo& player : players) {
			if (player.startedDefending) {
				restartMeasuringFrameAdvantage(player.index);
				restartMeasuringLandingFrameAdvantage(player.index);
			}
		}
		
		Entity superflashInstigator = getSuperflashInstigator();
		if (!superflashInstigator) {
			for (ProjectileInfo& projectile : projectiles) {
				if (!projectile.startedUp) {
					++projectile.startup;
				}
				++projectile.total;
			}
		}
	}
	
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		if (isEntityAlreadyDrawn(ent)) continue;

		bool active = ent.isActive();
		logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));
		
		size_t hurtboxesPrevSize = graphics.drawDataPrepared.hurtboxes.size();
		size_t hitboxesPrevSize = graphics.drawDataPrepared.hitboxes.size();
		size_t pointsPrevSize = graphics.drawDataPrepared.points.size();
		size_t pushboxesPrevSize = graphics.drawDataPrepared.pushboxes.size();
		
		int hitboxesCount = 0;
		DrawHitboxArrayCallParams hurtbox;
		collectHitboxes(ent, active, &hurtbox, &graphics.drawDataPrepared.hitboxes, &graphics.drawDataPrepared.points, &graphics.drawDataPrepared.pushboxes, &hitboxesCount);
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
			collectHitboxes(attached, active, &hurtbox, &graphics.drawDataPrepared.hitboxes, &graphics.drawDataPrepared.points, &graphics.drawDataPrepared.pushboxes, &hitboxesCount);
			graphics.drawDataPrepared.hurtboxes.push_back({ false, hurtbox });
			drawnEntities.push_back(attached);
		}
		if ((team == 0 || team == 1) && frameHasChanged) {
			if (hitboxesCount) {
				if (i < 2) {
					PlayerInfo& player = players[team];
					player.hitboxesCount += hitboxesCount;
				} else if (frameHasChanged) {
					ProjectileInfo& projectile = findProjectile(ent);
					if (projectile.ptr) {
						projectile.markActive = true;
					}
				}
			}
		}
		if (invisChipp.needToHide(ent)) {
			graphics.drawDataPrepared.hurtboxes.resize(hurtboxesPrevSize);
			graphics.drawDataPrepared.hitboxes.resize(hitboxesPrevSize);
			graphics.drawDataPrepared.points.resize(pointsPrevSize);
			graphics.drawDataPrepared.pushboxes.resize(pushboxesPrevSize);
		}
	}
	
	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
	if (frameHasChanged) {
		Entity superflashInstigator = getSuperflashInstigator();
		int instigatorTeam = -1;
		if (superflashInstigator) {
			instigatorTeam = superflashInstigator.team();
		}
		if (!superflashInstigator) {
			for (PlayerInfo& player : players) {
				player.activesProj.beginMergeFrame();
			}
		}
		for (ProjectileInfo& projectile : projectiles) {
			if (!projectile.markActive) {
				if (!superflashInstigator) {
					projectile.actives.addNonActive();
				}
			} else if (!superflashInstigator || !projectile.startedUp) {
				if (!projectile.startedUp) {
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1)) {
						PlayerInfo& player = players[projectile.team];
						if (!player.startupProj) {
							player.startupProj = projectile.startup;
						}
					}
					projectile.startedUp = true;
				}
				projectile.actives.addActive(projectile.hitNumber, 1);
				if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1)) {
					PlayerInfo& player = players[projectile.team];
					if (superflashInstigator) {
						player.activesProj.addSuperfreezeActive(projectile.hitNumber);
					} else {
						player.activesProj.addActive(projectile.hitNumber, 1);
					}
				}
			}
		}
		if (!superflashInstigator) {
			for (PlayerInfo& player : players) {
				player.activesProj.endMergeFrame();
			}
		}
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = players[i];
			PlayerInfo& other = players[1 - i];
			
			if (!superflashInstigator) {
				++player.timePassed;
				++player.timePassedLanding;
				if (player.landingOrPreJump) {
					++player.landingOrPreJumpFrames;
				}
				if (player.timeSinceLastGap == 0) {
					if (other.idle || !other.onTheDefensive) {
						player.timeSinceLastGap = 1;
					}
				} else {
					++player.timeSinceLastGap;
				}
				
				if (player.idlePlus && !other.idlePlus) {
					if (measuringFrameAdvantage) {
						++player.frameAdvantage;
					}
				} else if (!player.idlePlus && other.idlePlus) {
					if (measuringFrameAdvantage) {
						--player.frameAdvantage;
					}
				} else if (!player.idlePlus && !other.idlePlus) {
					if (measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
					}
					player.frameAdvantage = 0;
					other.frameAdvantage = 0;
					measuringFrameAdvantage = true;
					player.frameAdvantageValid = false;
					other.frameAdvantageValid = false;
				} else if (player.idlePlus && other.idlePlus) {
					if (measuringFrameAdvantage) {
						measuringFrameAdvantage = false;
						player.frameAdvantageValid = true;
						other.frameAdvantageValid = true;
					}
				}
				
				if (i == measuringLandingFrameAdvantage) {
					if (player.idleLanding && !other.idle) {
						++player.landingFrameAdvantage;
						--other.landingFrameAdvantage;
					} else if (!player.idleLanding && other.idle) {
						--player.landingFrameAdvantage;
						++other.landingFrameAdvantage;
					} else if (!player.idleLanding && !other.idle) {
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
					} else if (player.idleLanding && other.idle) {
						measuringLandingFrameAdvantage = -1;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					}
				}
				
			}
			Entity ent = entityList.slots[i];
			bool hasHitboxes = player.hitboxesCount > 0;
			if (!player.hitstop
					&& !(superflashInstigator && superflashInstigator != ent)
					&& (
						player.cmnActIndex == CmnActLandingStiff
						|| !player.idle
						&& !player.isLandingOrPreJump
						&& player.moveOriginatedInTheAir
						&& ent.y() == 0
						&& ent.physicsYImpulse() == 0
						&& player.startedUp
						&& !hasHitboxes
					)) {
				++player.landingRecovery;
			} else if (!player.hitstop
					&& !(superflashInstigator && superflashInstigator != ent)
					&& player.cmnActIndex != CmnActJump
					&& !player.isLanding) {
				if (!player.idlePlus && !player.startedUp && !superflashInstigator) {
					++player.startup;
					++player.total;
				}
				if (superflashInstigator == ent && !player.superfreezeStartup) {
					player.superfreezeStartup = player.total;
				}
				if (hasHitboxes) {
					if (!player.startedUp) {
						player.startedUp = true;
						player.actives.addActive(ent.currentHitNum());
					} else if (!superflashInstigator) {
						if (player.recovery) {
							player.actives.addNonActive(player.recovery);
							player.recovery = 0;
						}
						player.actives.addActive(ent.currentHitNum());
						++player.total;
					}
				} else if (!player.idlePlus && !superflashInstigator && player.startedUp) {
					++player.total;
					++player.recovery;
				}
			}
		}
		
		for (PlayerInfo& player : players) {
			player.startupDisp = 0;
			player.activesDisp.clear();
			player.recoveryDisp = 0;
			player.totalDisp = 0;
			if (player.startedUp && !player.startupProj) {
				player.startupDisp = player.startup;
				player.activesDisp = player.actives;
				player.recoveryDisp = player.recovery;
				player.totalDisp = player.total;
			} else if (player.startupProj && !player.startedUp) {
				player.startupDisp = player.startupProj;
				player.activesDisp = player.activesProj;
				int activesDispTotal = player.activesDisp.total();
				if (player.startupDisp + activesDispTotal - 1 >= player.total) {
					player.recoveryDisp = 0;
				} else {
					player.recoveryDisp = player.total - (player.startupDisp + activesDispTotal - 1);
				}
				player.totalDisp = player.total;
			} else if (player.startedUp && player.startupProj) {
				if (player.startup <= player.startupProj) {
					player.startupDisp = player.startup;
					player.activesDisp = player.actives;
					player.activesDisp.mergeTimeline(player.startupProj - player.startup + 1, player.activesProj);
					int activesDispTotal = player.activesDisp.total();
					if (player.startup + activesDispTotal - 1 >= player.total) {
						player.recoveryDisp = 0;
					} else {
						player.recoveryDisp = player.total - (player.startup + activesDispTotal - 1);
					}
					player.totalDisp = player.total;
				} else {
					player.startupDisp = player.startupProj;
					player.activesDisp = player.activesProj;
					player.activesDisp.mergeTimeline(player.startup - player.startupProj + 1, player.actives);
					int activesDispTotal = player.activesDisp.total();
					if (player.startupProj + activesDispTotal - 1 >= player.total) {
						player.recoveryDisp = 0;
					} else {
						player.recoveryDisp = player.total - (player.startupProj + activesDispTotal - 1);
					}
					player.totalDisp = player.total;
				}
			} else {
				player.totalDisp = player.total;
			}
			
			if (player.hitstop) {
				player.displayHitstop = true;
			}
			if (player.hitstun) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT;
			} else if (player.blockstun) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK;
			}
		}
		
		needFrameCleanup = true;
	}
	
#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}

void EndScene::FUpdatePrimitiveTransformCommand_ApplyHook(char* thisArg) {
	if (!shutdown && !graphics.shutdown) {
		std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
		if (!graphics.drawDataPrepared.empty && graphics.needNewDrawData) {
			graphics.drawDataUse.clear();
			graphics.drawDataPrepared.copyTo(&graphics.drawDataUse);
			graphics.drawDataPrepared.empty = true;
			graphics.needNewDrawData = false;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_FUpdatePrimitiveTransformCommand_ApplyMutex);
		orig_FUpdatePrimitiveTransformCommand_Apply(thisArg);
	}
}

bool EndScene::isEntityAlreadyDrawn(const Entity& ent) const {
	return std::find(drawnEntities.cbegin(), drawnEntities.cend(), ent) != drawnEntities.cend();
}

void EndScene::endSceneHook(IDirect3DDevice9* device) {
	graphics.graphicsThreadId = GetCurrentThreadId();
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
	
	ui.onEndScene(device);
}

void EndScene::processKeyStrokes() {
	bool trainingMode = game.isTrainingMode();
	keyboard.updateKeyStatuses();
	bool stateChanged;
	{
		stateChanged = ui.stateChanged;
		ui.stateChanged = false;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggle) || stateChanged && ui.gifModeOn != gifMode.gifModeOn)) {
		// idk how atomic_bool reacts to ! and operator bool(), so we do it the arduous way
		if (gifMode.gifModeOn == true) {
			gifMode.gifModeOn = false;
			logwrap(fputs("GIF mode turned off\n", logfile));
			needToRunNoGravGifMode = needToRunNoGravGifMode || (*aswEngine != nullptr);
		} else if (trainingMode) {
			gifMode.gifModeOn = true;
			logwrap(fputs("GIF mode turned on\n", logfile));
		}
		ui.gifModeOn = gifMode.gifModeOn;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleBackgroundOnly) || stateChanged && ui.gifModeToggleBackgroundOnly != gifMode.gifModeToggleBackgroundOnly)) {
		if (gifMode.gifModeToggleBackgroundOnly == true) {
			gifMode.gifModeToggleBackgroundOnly = false;
			logwrap(fputs("GIF mode (darken background only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleBackgroundOnly = true;
			logwrap(fputs("GIF mode (darken background only) turned on\n", logfile));
		}
		ui.gifModeToggleBackgroundOnly = gifMode.gifModeToggleBackgroundOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleCameraCenterOnly) || stateChanged && ui.gifModeToggleCameraCenterOnly != gifMode.gifModeToggleCameraCenterOnly)) {
		if (gifMode.gifModeToggleCameraCenterOnly == true) {
			gifMode.gifModeToggleCameraCenterOnly = false;
			logwrap(fputs("GIF mode (center camera only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleCameraCenterOnly = true;
			logwrap(fputs("GIF mode (center camera only) turned on\n", logfile));
		}
		ui.gifModeToggleCameraCenterOnly = gifMode.gifModeToggleCameraCenterOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleHideOpponentOnly) || stateChanged && ui.gifModeToggleHideOpponentOnly != gifMode.gifModeToggleHideOpponentOnly)) {
		if (gifMode.gifModeToggleHideOpponentOnly == true) {
			gifMode.gifModeToggleHideOpponentOnly = false;
			logwrap(fputs("GIF mode (hide opponent only) turned off\n", logfile));
			needToRunNoGravGifMode = true;
		}
		else if (trainingMode) {
			gifMode.gifModeToggleHideOpponentOnly = true;
			logwrap(fputs("GIF mode (hide opponent only) turned on\n", logfile));
		}
		ui.gifModeToggleHideOpponentOnly = gifMode.gifModeToggleHideOpponentOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.gifModeToggleHudOnly) || stateChanged && ui.gifModeToggleHudOnly != gifMode.gifModeToggleHudOnly)) {
		if (gifMode.gifModeToggleHudOnly == true) {
			gifMode.gifModeToggleHudOnly = false;
			logwrap(fputs("GIF mode (hide hud only) turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.gifModeToggleHudOnly = true;
			logwrap(fputs("GIF mode (hide hud only) turned on\n", logfile));
		}
		ui.gifModeToggleHudOnly = gifMode.gifModeToggleHudOnly;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.noGravityToggle) || stateChanged && ui.noGravityOn != gifMode.noGravityOn)) {
		if (gifMode.noGravityOn == true) {
			gifMode.noGravityOn = false;
			logwrap(fputs("No gravity mode turned off\n", logfile));
		}
		else if (trainingMode) {
			gifMode.noGravityOn = true;
			logwrap(fputs("No gravity mode turned on\n", logfile));
		}
		ui.noGravityOn = gifMode.noGravityOn;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.freezeGameToggle) || stateChanged && ui.freezeGame != freezeGame)) {
		if (freezeGame == true) {
			freezeGame = false;
			logwrap(fputs("Freeze game turned off\n", logfile));
		}
		else if (trainingMode) {
			freezeGame = true;
			logwrap(fputs("Freeze game turned on\n", logfile));
		}
		ui.freezeGame = freezeGame;
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.slowmoGameToggle) || stateChanged && ui.slowmoGame != game.slowmoGame)) {
		if (game.slowmoGame == true) {
			game.slowmoGame = false;
			logwrap(fputs("Slowmo game turned off\n", logfile));
		}
		else if (trainingMode) {
			game.slowmoGame = true;
			logwrap(fputs("Slowmo game turned on\n", logfile));
		}
		ui.slowmoGame = game.slowmoGame;
	}
	if (keyboard.gotPressed(settings.disableModKeyCombo)) {
		if (gifMode.modDisabled == true) {
			gifMode.modDisabled = false;
			logwrap(fputs("Mod enabled\n", logfile));
		} else {
			gifMode.modDisabled = true;
			logwrap(fputs("Mod disabled\n", logfile));
			needToRunNoGravGifMode = needToRunNoGravGifMode || (*aswEngine != nullptr);
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.disableHitboxDisplayToggle) || stateChanged && ui.hitboxDisplayDisabled != gifMode.hitboxDisplayDisabled)) {
		if (gifMode.hitboxDisplayDisabled == true) {
			gifMode.hitboxDisplayDisabled = false;
			logwrap(fputs("Hitbox display enabled\n", logfile));
		} else {
			gifMode.hitboxDisplayDisabled = true;
			logwrap(fputs("Hitbox display disabled\n", logfile));
		}
		ui.hitboxDisplayDisabled = gifMode.hitboxDisplayDisabled;
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.modWindowVisibilityToggle)) {
		if (ui.visible == true) {
			ui.visible = false;
			logwrap(fputs("UI display disabled\n", logfile));
		} else {
			ui.visible = true;
			logwrap(fputs("UI display enabled\n", logfile));
		}
	}
	if (!gifMode.modDisabled && (keyboard.gotPressed(settings.continuousScreenshotToggle) || stateChanged && ui.continuousScreenshotToggle != continuousScreenshotMode)) {
		if (continuousScreenshotMode) {
			continuousScreenshotMode = false;
			logwrap(fputs("Continuous screenshot mode off\n", logfile));
		} else if (trainingMode) {
			continuousScreenshotMode = true;
			logwrap(fputs("Continuous screenshot mode on\n", logfile));
		}
		ui.continuousScreenshotToggle = continuousScreenshotMode;
	}
	if (!gifMode.modDisabled && keyboard.gotPressed(settings.screenshotBtn)) {
		ui.takeScreenshotPress = true;
		ui.takeScreenshotTimer = 10;
	}
	if (!gifMode.modDisabled) {
		for (int i = 0; i < 2; ++i) {
			if (ui.clearTensionGainMaxCombo[i]) {
				ui.clearTensionGainMaxCombo[i] = false;
				players[i].tensionGainMaxCombo = 0;
				players[i].burstGainMaxCombo = 0;
				players[i].tensionGainLastCombo = 0;
				players[i].burstGainLastCombo = 0;
			}
		}
	}
}

void EndScene::actUponKeyStrokesThatAlreadyHappened() {
	bool trainingMode = game.isTrainingMode();
	bool allowNextFrameIsHeld = false;
	if (!gifMode.modDisabled) {
		allowNextFrameIsHeld = keyboard.isHeld(settings.allowNextFrameKeyCombo);
	}
	if (ui.allowNextFrame && !gifMode.modDisabled) {
		allowNextFrameIsHeld = true;
		ui.allowNextFrame = false;
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
	if (!gifMode.modDisabled && ui.takeScreenshotPress) {
		ui.takeScreenshotPress = false;
		if (butDontPrepareBoxData) {
			std::unique_lock<std::mutex> specialGuard(graphics.specialScreenshotFlagMutex);
			graphics.specialScreenshotFlag = true;
		} else {
			graphics.drawDataPrepared.needTakeScreenshot = true;
		}
	}
	bool screenshotPathEmpty = false;
	{
		std::unique_lock<std::mutex> guard(settings.screenshotPathMutex);
		screenshotPathEmpty = settings.screenshotPath.empty();
	}
	needContinuouslyTakeScreens = false;
	if (!gifMode.modDisabled
			&& ((keyboard.isHeld(settings.screenshotBtn) || ui.takeScreenshot) && settings.allowContinuousScreenshotting || continuousScreenshotMode)
			&& trainingMode
			&& !screenshotPathEmpty) {
		needContinuouslyTakeScreens = true;
	}
	game.freezeGame = (allowNextFrameIsHeld || freezeGame) && trainingMode && !gifMode.modDisabled;
	if (!trainingMode || gifMode.modDisabled) {
		gifMode.gifModeOn = false;
		ui.gifModeOn = false;
		gifMode.noGravityOn = false;
		ui.noGravityOn = false;
		game.slowmoGame = false;
		ui.slowmoGame = false;
		gifMode.gifModeToggleBackgroundOnly = false;
		ui.gifModeToggleBackgroundOnly = false;
		gifMode.gifModeToggleCameraCenterOnly = false;
		ui.gifModeToggleCameraCenterOnly = false;
		gifMode.gifModeToggleHideOpponentOnly = false;
		ui.gifModeToggleHideOpponentOnly = false;
		gifMode.gifModeToggleHudOnly = false;
		ui.gifModeToggleHudOnly = false;
		clearContinuousScreenshotMode();
	}
	if (needToRunNoGravGifMode) {
		entityList.populate();
		noGravGifMode();
	}
	needToRunNoGravGifMode = false;
	
}

void EndScene::noGravGifMode() {
	char playerIndex;
	playerIndex = game.getPlayerSide();
	if (playerIndex == 2) playerIndex = 0;

	bool useGifMode = (gifMode.gifModeOn || gifMode.gifModeToggleHideOpponentOnly) && game.isTrainingMode();
	if (useGifMode) {
		for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
			it->wasFoundOnThisFrame = false;
		}
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			if (ent.team() != playerIndex) {
				hideEntity(ent);
			}
		}
		auto it = hiddenEntities.begin();
		while (it != hiddenEntities.end()) {
			if (!it->wasFoundOnThisFrame) {
				it = hiddenEntities.erase(it);
			}
			else {
				++it;
			}
		}
	} else {
		for (int i = 0; i < entityList.count; ++i) {
			Entity ent = entityList.list[i];
			auto found = findHiddenEntity(ent);
			if (found != hiddenEntities.end()) {
				const int currentScaleX = ent.scaleX();
				const int currentScaleY = ent.scaleY();
				const int currentScaleZ = ent.scaleZ();
				const int currentScaleDefault = ent.scaleDefault();
				const int currentScaleForParticles = ent.scaleForParticles();

				if (currentScaleX == 0) {
					ent.scaleX() = found->scaleX;
				}
				if (currentScaleY == 0) {
					ent.scaleY() = found->scaleY;
				}
				if (currentScaleZ == 0) {
					ent.scaleZ() = found->scaleZ;
				}
				if (currentScaleDefault == 0) {
					ent.scaleDefault() = found->scaleDefault;
					ent.scaleDefault2() = found->scaleDefault;
				}
				if (currentScaleForParticles == 0) {
					ent.scaleForParticles() = found->scaleForParticles;
				}
			}
		}
		hiddenEntities.clear();
	}

	bool useNoGravMode = gifMode.noGravityOn && game.isTrainingMode();
	if (useNoGravMode) {
		entityList.slots[playerIndex].physicsYImpulse() = 0;
	}
}

void EndScene::clearContinuousScreenshotMode() {
	continuousScreenshotMode = false;
	ui.continuousScreenshotToggle = false;
	previousTimeOfTakingScreen = ~0;
}

std::vector<EndScene::HiddenEntity>::iterator EndScene::findHiddenEntity(const Entity& ent) {
	for (auto it = hiddenEntities.begin(); it != hiddenEntities.end(); ++it) {
		if (it->ent == ent) {
			return it;
		}
	}
	return hiddenEntities.end();
}

// Called at the start of the tick.
// We use this merely to synchronize draw data between our logic tick hook and camera update function hook.
// They're called on the same thread.
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

LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return endScene.WndProcHook(hWnd, message, wParam, lParam);
}

// The game logic thread runs WndProc, by calling DispatchMessageW inbetween ticks.
LRESULT EndScene::WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	HookGuard hookGuard("WndProc");
	if (!shutdown) {
		if (!ui.shutdownGraphics && ui.WndProc(hWnd, message, wParam, lParam)) {
			return TRUE;
		}
		
		if (message == WM_DESTROY) {
			keyboard.thisProcessWindow = NULL;
		}
		
		if (message == WM_APP_SETTINGS_FILE_UPDATED) {
			settings.readSettings(true);
		}
		
		if (message == WM_KEYDOWN
				|| message == WM_KEYUP
				|| message == WM_SYSKEYDOWN
				|| message == WM_SYSKEYUP
				|| message == WM_APP_UI_STATE_CHANGED && wParam && ui.stateChanged) {
			processKeyStrokes();
		}
	}
	
	bool iLockedTheMutex = false;
	if (!orig_WndProcMutexLockedByWndProc) {
		orig_WndProcMutex.lock();
		orig_WndProcMutexLockedByWndProc = true;
		iLockedTheMutex = true;
	}
	LRESULT result = orig_WndProc(hWnd, message, wParam, lParam);
	if (iLockedTheMutex) {
		orig_WndProcMutex.unlock();
		orig_WndProcMutexLockedByWndProc = false;
	}
	return result;
}

// Training HUD here means:
// 1) The texts displaying hit dmg, combo dmg, max combo dmg.
// 2) Input history
// 3) Virtual sticks
// 4) Dummy status
// All drawn in this one function depending on in-game settings. This function hooks that.
void EndScene::drawTrainingHudHook(char* thisArg) {
	if (!shutdown && !graphics.shutdown) {
		if (gifMode.gifModeToggleHudOnly || gifMode.gifModeOn) return;
		drawTexts();
	}
	{
		std::unique_lock<std::mutex> guard(orig_drawTrainingHudMutex);
		orig_drawTrainingHud(thisArg);
	}
}

void EndScene::drawTexts() {
	//return;
	#if 1
	// Example code
	{
		char HelloWorld[] = "oig^mAtk1;";
		DrawTextWithIconsParams s;
	    s.field159_0x100 = 36.0;
	    s.field11_0x2c = 177;
	    s.field160_0x104 = -1.0;
	    s.field4_0x10 = -1.0;
	    s.field155_0xf0 = 1;
	    s.field157_0xf8 = -1;
	    s.field158_0xfc = -1;
	    s.field161_0x108 = 0;
	    s.field162_0x10c = 0;
	    s.field163_0x110 = -1;
	    s.field164_0x114 = 0;
	    s.field165_0x118 = 0;
	    s.field166_0x11c = -1;
	    s.outlineColor = 0xff000000;
	    s.flags2 = 0xff000000;
	    s.x = 100;
	    s.y = 185.0 + 34 * 3;
	    s.alignment = ALIGN_LEFT;
	    s.text = HelloWorld;
	    s.field156_0xf4 = 0x210;
	    drawTextWithIcons(&s,0x0,1,4,0,0);
	}
    return;
	{
		char HelloWorld[] = "-123765";
		DrawTextWithIconsParams s;
	    s.field159_0x100 = 36.0;
	    s.field11_0x2c = 177;
	    s.field160_0x104 = -1.0;
	    s.field4_0x10 = -1.0;
	    s.field155_0xf0 = 1;
	    s.field157_0xf8 = -1;
	    s.field158_0xfc = -1;
	    s.field161_0x108 = 0;
	    s.field162_0x10c = 0;
	    s.field163_0x110 = -1;
	    s.field164_0x114 = 0;
	    s.field165_0x118 = 0;
	    s.field166_0x11c = -1;
	    s.outlineColor = 0xff000000;
	    s.flags2 = 0xff000000;
	    s.x = 460;
	    s.y = 185.0 + 34 * 3;
	    s.alignment = ALIGN_CENTER;
	    s.text = HelloWorld;
	    s.field156_0xf4 = 0x210;
	    drawTextWithIcons(&s,0x0,1,4,0,0);
	}
	#endif
}

// Called when switching characters, exiting the match.
void EndScene::onAswEngineDestroyed() {
	{
		std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
		graphics.drawDataPrepared.clear();
	}
	clearContinuousScreenshotMode();
	measuringFrameAdvantage = false;
	measuringLandingFrameAdvantage = -1;
	memset(tensionGainOnLastHit, 0, sizeof tensionGainOnLastHit);
	memset(burstGainOnLastHit, 0, sizeof burstGainOnLastHit);
	memset(tensionGainOnLastHitUpdated, 0, sizeof tensionGainOnLastHitUpdated);
	memset(burstGainOnLastHitUpdated, 0, sizeof burstGainOnLastHitUpdated);
	registeredHits.clear();
	for (int i = 0; i < 2; ++i) {
		players[i].clear();
	}
	projectiles.clear();
	needFrameCleanup = false;
}

// When someone YRCs, PRCs, RRCs or does a super, the address of their entity is written into the
// *aswEngine + superflashInstigatorOffset variable for the duration of the superfreeze.
Entity EndScene::getSuperflashInstigator() {
	if (!superflashInstigatorOffset || !*aswEngine) return nullptr;
	return Entity{ *(char**)(*aswEngine + superflashInstigatorOffset) };
}

int EndScene::getSuperflashCounterAll() {
	if (!superflashCounterAllOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterAllOffset);
}

int EndScene::getSuperflashCounterSelf() {
	if (!superflashCounterSelfOffset || !*aswEngine) return 0;
	return *(int*)(*aswEngine + superflashCounterSelfOffset);
}

void EndScene::restartMeasuringFrameAdvantage(int index) {
	PlayerInfo& player = players[index];
	PlayerInfo& other = players[1 - index];
	player.frameAdvantageValid = false;
	other.frameAdvantageValid = false;
	if (other.idlePlus) {
		player.frameAdvantage = -other.timePassed;
		other.frameAdvantage = other.timePassed;
	}
	measuringFrameAdvantage = true;
}

void EndScene::restartMeasuringLandingFrameAdvantage(int index) {
	PlayerInfo& player = players[index];
	PlayerInfo& other = players[1 - index];
	player.landingFrameAdvantageValid = false;
	other.landingFrameAdvantageValid = false;
	if (other.idleLanding) {
		player.landingFrameAdvantage = -other.timePassedLanding;
		other.landingFrameAdvantage = other.timePassedLanding;
	}
	measuringLandingFrameAdvantage = 1 - index;
}

// There're three hit detection calls: with hitDetectionType 0, 1 and 2, called
// one right after the other each logic tick.
// A single hit detection does a loop in a loop and tries to find which two entities hit each other.
// We use this hook at the start of hit detection algorithm to measure some values before a hit.
void EndScene::onHitDetectionStart(int hitDetectionType) {
	if (hitDetectionType == 0) {
		entityList.populate();
		for (int i = 0; i < 2; ++i) {
			Entity ent = entityList.slots[i];
			tensionRecordedHit[i] = ent.tension();
			burstRecordedHit[i] = game.getBurst(i);
			tensionGainOnLastHit[i] = 0;
			burstGainOnLastHit[i] = 0;
			tensionGainOnLastHitUpdated[i] = false;
			burstGainOnLastHitUpdated[i] = false;
		}
		registeredHits.clear();
	}
}

// We use this hook at the end of hit detection algorithm to measure some values after a hit.
void EndScene::onHitDetectionEnd(int hitDetectionType) {
	entityList.populate();
	for (int i = 0; i < 2; ++i) {
		Entity ent = entityList.slots[i];
		int tension = ent.tension();
		int tensionBefore = tensionRecordedHit[i];
		if (tension != tensionBefore) {
			tensionGainOnLastHit[i] += tension - tensionBefore;
			tensionGainOnLastHitUpdated[i] = true;
		}
		tensionRecordedHit[i] = tension;
		int burst = game.getBurst(i);
		int burstBefore = burstRecordedHit[i];
		if (burst != burstBefore) {
			burstGainOnLastHit[i] += burst - burstBefore;
			burstGainOnLastHitUpdated[i] = true;
		}
		burstRecordedHit[i] = burst;
	}
}

// Called by a hook inside hit detection when a hit was detected.
void EndScene::registerHit(HitResult hitResult, bool hasHitbox, Entity attacker, Entity defender) {
	registeredHits.emplace_back();
	RegisteredHit& hit = registeredHits.back();
	hit.projectile.fill(attacker);
	hit.isPawn = attacker.isPawn();
	hit.hitResult = hitResult;
	hit.hasHitbox = hasHitbox;
	hit.attacker = attacker;
	hit.defender = defender;
	if (defender.isPawn()) {
		PlayerInfo& defenderPlayer = findPlayer(defender);
		defenderPlayer.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
	}
	if (hasHitbox) {
		ProjectileInfo& projectile = findProjectile(attacker);
		if (projectile.ptr) {
			projectile.fill(attacker);
			projectile.hitstop = attacker.hitstop();
			projectile.landedHit = true;
			projectile.markActive = true;
		}
	}
}

// Called at the start of an UE3 engine tick.
// This tick runs even when paused or not in a match.
void EndScene::onUWorld_TickBegin() {
	logicThreadId = GetCurrentThreadId();
	drawDataPrepared = false;
}

// Called at the end of an UE3 engine tick.
// This tick runs even when paused or not in a match.
void EndScene::onUWorld_Tick() {
	if (!drawDataPrepared) {
		ui.drawData = nullptr;
		ui.needInitFont = false;
		if (!ui.shutdownGraphics) {
			ui.prepareDrawData();
		}
	}
	if (shutdown) {
		if (*aswEngine) {
			bool needToCallNoGravGifMode = gifMode.gifModeOn
				|| gifMode.gifModeToggleHideOpponentOnly
				|| gifMode.noGravityOn;
			gifMode.gifModeOn = false;
			gifMode.noGravityOn = false;
			gifMode.gifModeToggleHideOpponentOnly = false;
			if (needToCallNoGravGifMode && game.isTrainingMode()) {
				entityList.populate();
				noGravGifMode();
			}
		}
		hud.onDllDetach();
		ui.onDllDetachNonGraphics();
		SetEvent(shutdownFinishedEvent);
	}
}

void EndScene::HookHelp::endSceneCallerHook(int param1, int param2, int param3) {
	HookGuard hookGuard("endSceneCaller");
	IDirect3DDevice9* device = *(IDirect3DDevice9**)((char*)this + 0x24);
	if (!endScene.shutdown && !graphics.shutdown) {
		endScene.endSceneHook(device);
	} else {
		graphics.onEndSceneStart(device);
	}
	{
		std::unique_lock<std::mutex> guard(endScene.orig_endSceneCallerMutex);
		endScene.orig_endSceneCaller((void*)this, param1, param2, param3);
	}
	if (!endScene.shutdown && !graphics.shutdown) {
		{
			std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
			graphics.needNewDrawData = true;
		}
		{
			std::unique_lock<std::mutex> guard(camera.valuesPrepareMutex);
			graphics.needNewCameraData = true;
		}
	}
}

void EndScene::HookHelp::BBScr_createObjectWithArgHook(const char* animName, unsigned int posType) {
	HookGuard hookGuard("BBScr_createObjectWithArg");
	if (!endScene.shutdown) {
		endScene.creatingObject = true;
		endScene.createdObjectAnim = animName;
		endScene.creatorOfCreatedObject = Entity{(char*)this};
	}
	{
		static bool imHoldingTheLock = false;
		bool needUnlock = false;
		if (!imHoldingTheLock) {
			needUnlock = true;
			imHoldingTheLock = true;
			endScene.orig_BBScr_createObjectWithArgMutex.lock();
		}
		endScene.orig_BBScr_createObjectWithArg(this, animName, posType);
		if (needUnlock) {
			imHoldingTheLock = false;
			endScene.orig_BBScr_createObjectWithArgMutex.unlock();
		}
	}
}

void EndScene::onObjectCreated(Entity pawn, Entity createdPawn, const char* animName) {
	if (!gifMode.modDisabled && (gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && game.isTrainingMode()) {
		int playerSide = game.getPlayerSide();
		if (playerSide == 2) playerSide = 0;
		if (createdPawn.team() != playerSide) {
			hideEntity(createdPawn);
		}
	}
	for (auto it = projectiles.begin(); it != projectiles.end(); ++it) {
		if (it->ptr == createdPawn) {
			if (it->landedHit) {
				it->ptr = nullptr;
			} else {
				projectiles.erase(it);
			}
			break;
		}
	}
	projectiles.emplace_back();
	ProjectileInfo& projectile = projectiles.back();
	projectile.fill(createdPawn);
	bool ownerFound = false;
	ProjectileInfo& creatorProjectile = findProjectile(pawn);
	if (creatorProjectile.ptr) {
		projectile.startup = creatorProjectile.total;
		projectile.total = creatorProjectile.total;
		projectile.disabled = creatorProjectile.disabled;
		ownerFound = true;
	}
	if (!ownerFound && pawn.isPawn()) {
		entityList.populate();
		PlayerInfo& player = findPlayer(pawn);
		projectile.startup = player.total;
		projectile.total = player.total;
	}
}

void EndScene::hideEntity(Entity ent) {
	const int currentScaleX = ent.scaleX();
	const int currentScaleY = ent.scaleY();
	const int currentScaleZ = ent.scaleZ();
	const int currentScaleDefault = ent.scaleDefault();  // 0x2664 is another default scaling
	const int currentScaleForParticles = ent.scaleForParticles();  // 0x2618 is another default scaling used for created particles

	auto found = findHiddenEntity(ent);
	if (found == hiddenEntities.end()) {
		hiddenEntities.emplace_back();
		HiddenEntity& hiddenEntity = hiddenEntities.back();
		hiddenEntity.ent = ent;
		hiddenEntity.scaleX = currentScaleX;
		hiddenEntity.scaleY = currentScaleY;
		hiddenEntity.scaleZ = currentScaleZ;
		hiddenEntity.scaleDefault = currentScaleDefault;
		hiddenEntity.scaleForParticles = currentScaleForParticles;
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
		if (currentScaleForParticles != 0) {
			hiddenEntity.scaleForParticles = currentScaleForParticles;
		}
		hiddenEntity.wasFoundOnThisFrame = true;
	}
	ent.scaleX() = 0;
	ent.scaleY() = 0;
	ent.scaleZ() = 0;
	ent.scaleDefault() = 0;
	ent.scaleDefault2() = 0;
	ent.scaleForParticles() = 0;
}

bool EndScene::didHit(Entity attacker) {
	for (auto it = registeredHits.begin(); it != registeredHits.end(); ++it) {
		if (it->attacker == attacker) {
			return true;
		}
	}
	return false;
}

void EndScene::HookHelp::setAnimHook(const char* animName) {
	HookGuard hookGuard("setAnim");
	endScene.setAnimHook(Entity{(char*)this}, animName);
}

void EndScene::setAnimHook(Entity pawn, const char* animName) {
	if (pawn.isPawn() && !gifMode.modDisabled) {
		PlayerInfo& player = findPlayer(pawn);
		if (pawn.isIdle()) player.wasIdle = true;
	}
	{
		std::unique_lock<std::mutex> guard(orig_setAnimMutex);
		orig_setAnim((void*)pawn, animName);
	}
	if (pawn.isPawn()) {
		PlayerInfo& player = findPlayer(pawn);
		int blockstun = pawn.blockstun();
		if (blockstun) {
			// defender was observed to not be in hitstop at this point, but having blockstun nonetheless
			player.blockstunMax = blockstun;
			player.setBlockstunMax = true;
		}
	}
}

void EndScene::initializePawn(PlayerInfo& player, Entity ent) {
	player.pawn = ent;
	player.charType = ent.characterType();
	ent.getWakeupTimings(&player.wakeupTimings);
	player.idle = ent.isIdle();
	player.weight = ent.weight();
	player.maxHp = ent.maxHp();
	player.defenseModifier = ent.defenseModifier();
}

PlayerInfo& EndScene::findPlayer(Entity ent) {
	for (int i = 0; i < 2; ++i) {
		if (players[i].pawn == ent) {
			return players[i];
		}
	}
	return emptyPlayer;
}

void EndScene::HookHelp::BBScr_createParticleWithArgHook(const char* animName, unsigned int posType) {
	HookGuard hookGuard("BBScr_createParticleWithArg");
	endScene.BBScr_createParticleWithArgHook(Entity{(char*)this}, animName, posType);
}

void EndScene::BBScr_createParticleWithArgHook(Entity pawn, const char* animName, unsigned int posType) {
	if (!gifMode.modDisabled && (gifMode.gifModeToggleHideOpponentOnly || gifMode.gifModeOn) && game.isTrainingMode() && pawn.isPawn()) {
		int playerSide = game.getPlayerSide();
		if (playerSide == 2) playerSide = 0;
		if (pawn.team() != playerSide) {
			pawn.scaleForParticles() = 0;
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_createParticleWithArgMutex);
		orig_BBScr_createParticleWithArg((void*)pawn, animName, posType);
	}
}

// Called before a logic tick happens. Doesn't run when the pause menu is open or a match is not running.
void EndScene::onTickActors_FDeferredTickList_FGlobalActorIteratorBegin(bool isFrozen) {
	if (!isFrozen) {
		if (needFrameCleanup) {
			frameCleanup();
		}
		needFrameCleanup = false;
	}
}

// Stuff that runs at the start of a tick
void EndScene::frameCleanup() {
	entityList.populate();
	for (auto it = projectiles.begin(); it != projectiles.end();) {
		ProjectileInfo& projectile = *it;
		projectile.landedHit = false;
		bool found = false;
		if (projectile.ptr) {
			for (int i = 2; i < entityList.count; ++i) {
				if (entityList.list[i] == projectile.ptr) {
					found = true;
					break;
				}
			}
		}
		if (!found) {
			it = projectiles.erase(it);
			continue;
		}
		++it;
	}
	for (PlayerInfo& player : players) {
		player.setHitstopMax = false;
		player.setHitstunMax = false;
		player.setBlockstunMax = false;
		player.wasIdle = false;
		player.startedDefending = false;
	}
}

void EndScene::HookHelp::pawnInitializeHook(void* initializationParams) {
	HookGuard hookGuard("pawnInitialize");
	endScene.pawnInitializeHook(Entity{(char*)this}, initializationParams);
}

void EndScene::pawnInitializeHook(Entity createdObj, void* initializationParams) { 
	if (!shutdown && creatingObject) {
		creatingObject = false;
		onObjectCreated(creatorOfCreatedObject, createdObj, createdObjectAnim);
	}
	{
		static bool imHoldingTheLock = false;
		bool needUnlock = false;
		if (!imHoldingTheLock) {
			needUnlock = true;
			imHoldingTheLock = true;
			endScene.orig_pawnInitializeMutex.lock();
		}
		endScene.orig_pawnInitialize(createdObj.ent, initializationParams);
		if (needUnlock) {
			imHoldingTheLock = false;
			endScene.orig_pawnInitializeMutex.unlock();
		}
	}
}

void EndScene::HookHelp::logicOnFrameAfterHitHook(int param1, int param2) {
	HookGuard hookGuard("logicOnFrameAfterHit");
	endScene.logicOnFrameAfterHitHook(Entity{(char*)this}, param1, param2);
}

void EndScene::logicOnFrameAfterHitHook(Entity pawn, int param1, int param2) {
	bool functionWillNotDoAnything = !pawn.inPainNextFrame() && (!pawn.inUnknownNextFrame() || !pawn.inBlockstunNextFrame());
	{
		std::unique_lock<std::mutex> guard(orig_logicOnFrameAfterHitMutex);
		orig_logicOnFrameAfterHit((void*)pawn.ent, param1, param2);
	}
	if (pawn.isPawn() && !functionWillNotDoAnything) {
		PlayerInfo& player = findPlayer(pawn);
		player.hitstopMax = pawn.startingHitstop();
		player.hitstunMax = pawn.hitstun() - (player.hitstopMax ? 1 : 0);
		player.setHitstopMax = true;
		player.setHitstunMax = true;
	}
}

ProjectileInfo& EndScene::findProjectile(Entity pawn) {
	for (ProjectileInfo& projectile : projectiles) {
		if (projectile.ptr == pawn) {
			return projectile;
		}
	}
	return emptyProjectile;
}

void EndScene::HookHelp::BBScr_runOnObjectHook(int entityReference) {
	HookGuard hookGuard("BBScr_runOnObject");
	endScene.BBScr_runOnObjectHook(Entity{(char*)this}, entityReference);
}

void EndScene::BBScr_runOnObjectHook(Entity pawn, int entityReference) {
	{
		std::unique_lock<std::mutex> guard(orig_BBScr_runOnObjectMutex);
		orig_BBScr_runOnObject((void*)pawn.ent, entityReference);
	}
	if (!shutdown && pawn.isPawn()) {
		PlayerInfo& player = findPlayer(pawn);
		Entity objectBeingRunOn = pawn.currentRunOnObject();
		if (objectBeingRunOn) {
			if (!objectBeingRunOn.isPawn()) {
				ProjectileInfo& projectile = findProjectile(objectBeingRunOn);
				if (projectile.ptr) {
					projectile.disabled = false;
					projectile.startup = player.total;
					projectile.total = player.total;
				}
			}
		}
	}
}
