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

EndScene endScene;

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
	
	// SendUnrealPawnData is USkeletalMeshComponent::UpdateTransform()
	orig_SendUnrealPawnData = (SendUnrealPawnData_t)sigscanOffset(
		"GuiltyGearXrd.exe",
		"8b 0d ?? ?? ?? ?? 33 db 53 e8 ?? ?? ?? ?? f3 0f 10 80 24 04 00 00 f3 0f 5c 05 ?? ?? ?? ?? f3 0f 10 8e d0 01 00 00 0f 2f c8 76 05 8d 43 01 eb 02",
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
		"8b f1 8b 0e e8 ?? ?? ?? ?? 8b 06 8b 48 04 89 44 24 04 85 c9 74 1e 83 78 08 00 74 18 f6 81 84 00 00 00 20",
		{ -2 },
		&error, "ReadUnrealPawnData");

	if (orig_ReadUnrealPawnData) {
		void (HookHelp::*readUnrealPawnDataHookPtr)(void) = &HookHelp::readUnrealPawnDataHook;
		if (!detouring.attach(&(PVOID&)orig_ReadUnrealPawnData,
			(PVOID&)readUnrealPawnDataHookPtr,
			&orig_ReadUnrealPawnDataMutex,
			"ReadUnrealPawnData")) return false;
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
		endScene.orig_SendUnrealPawnData((char*)this);  // this method likes to call itself
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

void EndScene::HookHelp::drawTrainingHudHook() {
	++detouring.hooksCounter;
	detouring.markHookRunning("ReadUnrealPawnData", true);
	endScene.drawTrainingHudHook((char*)this);
	detouring.markHookRunning("ReadUnrealPawnData", false);
	--detouring.hooksCounter;
}

void EndScene::sendUnrealPawnDataHook(char* thisArg) {
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
		graphics.drawDataPrepared.clearWithoutPlayers();
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
	for (int i = 0; i < 2; ++i) {
		graphics.drawDataPrepared.players[i].tensionGainLastCombo = tensionGainLastCombo[i];
		graphics.drawDataPrepared.players[i].burstGainLastCombo = burstGainLastCombo[i];
		graphics.drawDataPrepared.players[i].tensionGainMaxCombo = tensionGainMaxCombo[i];
		graphics.drawDataPrepared.players[i].burstGainMaxCombo = burstGainMaxCombo[i];
	}
	if (!drawDataPrepared) {
		ui.drawData = nullptr;
		ui.needInitFont = false;
		ui.prepareDrawData();
		drawDataPrepared = true;
	}
}

void EndScene::prepareDrawData(bool* needClearHitDetection) {
	logOnce(fputs("endSceneHook called\n", logfile));
	invisChipp.onEndSceneStart();
	drawnEntities.clear();

	noGravGifMode();

	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	bool frameHasChangedForScreenshot = false;
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
		frameHasChangedForScreenshot = true;
	}
	if (needContinuouslyTakeScreens) {
		if (frameHasChangedForScreenshot) {
			graphics.drawDataPrepared.needTakeScreenshot = true;
		}
		p1PreviousTimeOfTakingScreen = p1CurrentTimer;
		p2PreviousTimeOfTakingScreen = p2CurrentTimer;
	}
	else if (frameHasChangedForScreenshot) {
		p1PreviousTimeOfTakingScreen = ~0;
		p2PreviousTimeOfTakingScreen = ~0;
	}
	DWORD aswEngineTickCount = *(DWORD*)(*aswEngine + 4 + game.aswEngineTickCountOffset);
	bool frameHasChanged = prevAswEngineTickCount != aswEngineTickCount;
	prevAswEngineTickCount = aswEngineTickCount;
	
	if (frameHasChanged) {
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = graphics.drawDataPrepared.players[i];
			for (int j = 0; j < player.activeProjectilesCount; ++j) {
				Entity activeProjectile = player.activeProjectiles[j];
				bool found = false;
				for (int k = 0; k < entityList.count; k++) {
					if (activeProjectile == entityList.list[k]) {
						found = true;
						break;
					}
				}
				if (!found) {
					player.removeActiveProjectile(j);
					--j;
				}
			}
		}
		
		int distanceBetweenPlayers = Entity{entityList.slots[0]}.posX() - Entity{entityList.slots[1]}.posX();
		if (distanceBetweenPlayers < 0) distanceBetweenPlayers = -distanceBetweenPlayers;
		
		bool comboStarted = false;
		for (int i = 0; i < 2; ++i) {
			Entity ent(entityList.slots[i]);
			PlayerInfo& player = graphics.drawDataPrepared.players[i];
			if (ent.inPain() && !player.inPain) {
				comboStarted = true;
				break;
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			Entity ent(entityList.slots[i]);
			Entity otherEnt(entityList.slots[1 - i]);
			PlayerInfo& player = graphics.drawDataPrepared.players[i];
			PlayerInfo& other = graphics.drawDataPrepared.players[1 - i];
			player.charType = ent.characterType();
			player.hp = ent.hp();
			player.maxHp = ent.maxHp();
			player.defenseModifier = ent.defenseModifier();
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
				player.wasCombod = player.inPain;
				if (tensionGainOnLastHitUpdated[i]) {
					tensionGainLastCombo[i] = tensionGainOnLastHit[i];
					tensionGain = player.tension - tensionRecordedHit[i];
				} else {
					tensionGainLastCombo[i] = 0;
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
				tensionGainLastCombo[i] += tensionGain;
				player.burstGainLastCombo += burstGain;
			}
			if (tensionGainLastCombo[i] > tensionGainMaxCombo[i]) {
				tensionGainMaxCombo[i] = tensionGainLastCombo[i];
			}
			if (player.burstGainLastCombo > burstGainMaxCombo[i]) {
				burstGainMaxCombo[i] = player.burstGainLastCombo;
			}
			player.tensionGainMaxCombo = tensionGainMaxCombo[i];
			player.burstGainMaxCombo = burstGainMaxCombo[i];
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
			if (player.hitstop == 0 && hitstop != 0 && ent.currentAnimDuration() != 1) {
				player.hitstop = 0;
			} else {
				player.hitstop = hitstop;
			}
			player.airborne = ent.y() > 0;  // there's also tumbling state in which you're airborne, check *(DWORD*)(end + 0x4d40) & 0x4 != 0 if you need it. This flag is set on any airborne type of hitstun
			player.nextHitstop = hitstop;
			player.weight = ent.weight();
			const char* animName = ent.animationName();
			ent.getWakeupTimings(&player.wakeupTimings);
			player.wakeupTiming = 0;
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			if (cmnActIndex == CmnActFDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceDown;
			} else if (cmnActIndex == CmnActBDown2Stand) {
				player.wakeupTiming = player.wakeupTimings.faceUp;
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
						player.startedUp = false;
						player.startup = player.landingOrPreJumpFrames;
						player.actives.clear();
						player.recovery = 0;
						player.total = player.landingOrPreJumpFrames;
						player.superfreezeStartup = 0;
						player.projectileStartedUp = false;
						player.projectileStartup = 0;
						player.projectileActives.clear();
						player.idlePlus = false;
						player.timePassed = player.landingOrPreJumpFrames;
						player.timePassedLanding = player.landingOrPreJumpFrames;
					}
					player.landingOrPreJumpFrames = 0;
				}
				player.landingOrPreJump = false;
			}
			memcpy(player.anim, animName, 32);
			++player.timeSinceLastBusyStart;
		}
		// I need another loop because in the next one each player refers to other's timePassed and I modified it
	
		for (int i = 0; i < 2; i++) {
			Entity ent(entityList.slots[i]);
			PlayerInfo& player = graphics.drawDataPrepared.players[i];
			PlayerInfo& other = graphics.drawDataPrepared.players[1 - i];
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			if (player.idleNext != player.idle) {
				player.idle = player.idleNext;
				if (!player.idle) {
					player.landingOrPreJump = player.isLandingOrPreJump;
					if (player.onTheDefensive) {
						restartMeasuringFrameAdvantage(i);
						restartMeasuringLandingFrameAdvantage(i);
						
						
						if (other.timeSinceLastGap > 45) {
							other.clearGaps();
						} else {
							other.addGap(other.timeSinceLastGap);
						}
						other.timeSinceLastGap = 0;
					}
					player.timeSinceLastBusyStart = 0;
					if (!player.landingOrPreJump) {
						player.startedUp = false;
						player.startup = 0;
						player.actives.clear();
						player.recovery = 0;
						player.total = 0;
						player.superfreezeStartup = 0;
						player.projectileStartedUp = false;
						player.projectileStartup = 0;
						player.projectileActives.clear();
					}
				} else if (player.airborne) {
					player.needLand = true;
					player.landingFrameAdvantageValid = false;
					other.landingFrameAdvantageValid = false;
				}
			}
			int remainingDoubleJumps = ent.remainingDoubleJumps();
			int remainingAirDashes = ent.remainingAirDashes();
			player.gettingUp = ent.gettingUp();
			if (player.needLand
					&& (player.isLanding
					|| player.onTheDefensive && !player.airborne)) {
				player.needLand = false;
			}
			if (player.idleLanding != player.idle) {
				if (!(!player.idleLanding && player.needLand)) {
					player.idleLanding = player.idle;
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
							measuringLandingFrameAdvantage = i;
							player.landingFrameAdvantageValid = false;
							other.landingFrameAdvantageValid = false;
							player.landingFrameAdvantage = 0;
							other.landingFrameAdvantage = 0;
						}
					}
					player.timePassedLanding = 0;
				}
			}
			player.remainingDoubleJumps = remainingDoubleJumps;
			player.remainingAirDashes = remainingAirDashes;
			bool idlePlus = player.idle || player.landingOrPreJump;
			if (idlePlus != player.idlePlus) {
				player.timePassed = 0;
				player.idlePlus = idlePlus;
			}
		}
	}
	
	for (int i = 0; i < entityList.count; i++)
	{
		Entity ent(entityList.list[i]);
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
			PlayerInfo& player = graphics.drawDataPrepared.players[team];
			if (hitboxesCount) {
				if (i < 2) {
					player.hitboxesCount += hitboxesCount;
				} else {
					player.addActiveProjectile(ent);
				}
			} else {
				int foundIndex = player.findActiveProjectile(ent);
				if (foundIndex != -1) {
					player.removeActiveProjectile(foundIndex);
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
	for (int i = 0; i < 2; ++i) {
		PlayerInfo& player = graphics.drawDataPrepared.players[i];
		PlayerInfo& other = graphics.drawDataPrepared.players[1 - i];
	}

	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHits();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrows();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
	if (frameHasChanged) {
		Entity superflashInstigator = getSuperflashInstigator();
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = graphics.drawDataPrepared.players[i];
			PlayerInfo& other = graphics.drawDataPrepared.players[1 - i];
			
			++player.timeSinceLastActiveProjectile;
			
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
			if (!player.hitstop && !(superflashInstigator && superflashInstigator != ent)) {
				if (!player.idlePlus && !player.startedUp && !superflashInstigator) {
					++player.startup;
					++player.total;
				}
				if (player.hasNewActiveProjectiles) {
					if (!player.projectileStartedUp || player.timeSinceLastActiveProjectile > 60) {
						player.projectileStartedUp = true;
						player.projectileStartup = player.timeSinceLastBusyStart;
						player.projectileActives.clear();
					}
					player.timeSinceLastActiveProjectile = 0;
					player.projectileActives.addActive();
				} else {
					player.projectileActives.addNonActive();
				}
				bool hasHitboxes = !player.idlePlus && (player.hitboxesCount || player.hasNewActiveProjectiles);
				if (superflashInstigator == ent && !player.superfreezeStartup) {
					player.superfreezeStartup = player.total;
				}
				if (hasHitboxes) {
					if (!player.startedUp) {
						player.startedUp = true;
						player.actives.addActive();
					} else if (!superflashInstigator) {
						if (player.recovery) {
							player.actives.addNonActive(player.recovery);
							player.recovery = 0;
						}
						player.actives.addActive();
						++player.total;
					}
				} else if (!player.idlePlus && !superflashInstigator && player.startedUp) {
					++player.total;
					++player.recovery;
				}
			}
			player.hitstop = player.nextHitstop;
		}
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
		} else {
			for (int i = 0; i < 2; ++i) {
				graphics.drawDataUse.players[i].tensionGainLastCombo = graphics.drawDataPrepared.players[i].tensionGainLastCombo;
				graphics.drawDataUse.players[i].burstGainLastCombo = graphics.drawDataPrepared.players[i].burstGainLastCombo;
				graphics.drawDataUse.players[i].tensionGainMaxCombo = graphics.drawDataPrepared.players[i].tensionGainMaxCombo;
				graphics.drawDataUse.players[i].burstGainMaxCombo = graphics.drawDataPrepared.players[i].burstGainMaxCombo;
			}
		}
	}
	{
		std::unique_lock<std::mutex> guard(orig_ReadUnrealPawnDataMutex);
		orig_ReadUnrealPawnData(thisArg);
	}
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

	bool doYourThing = !gifMode.hitboxDisplayDisabled && !ui.isSteamOverlayActive;

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
				tensionGainMaxCombo[i] = 0;
				burstGainMaxCombo[i] = 0;
				tensionGainLastCombo[i] = 0;
				burstGainLastCombo[i] = 0;
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
				const int currentScaleDefault = *(int*)(ent + 0x2594);  // 0x2664 is another default scaling

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
				*(int*)(ent + 0x2664) = 0;
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
					*(int*)(ent + 0x2664) = found->scaleDefault;
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
	ui.continuousScreenshotToggle = false;
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

LRESULT CALLBACK hook_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return endScene.WndProcHook(hWnd, message, wParam, lParam);
}

// WndProc runs the game logic
LRESULT EndScene::WndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	++detouring.hooksCounter;
	detouring.markHookRunning("WndProc", true);
	if (ui.WndProc(hWnd, message, wParam, lParam)) {
		detouring.markHookRunning("WndProc", false);
		--detouring.hooksCounter;
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
	
	bool iLockedTheMutex = false;
	if (!orig_WndProcMutexLockedByWndProc) {
		orig_WndProcMutex.lock();
		orig_WndProcMutexLockedByWndProc = true;
		wndProcThread = GetCurrentThreadId();
		iLockedTheMutex = true;
	}
	LRESULT result = orig_WndProc(hWnd, message, wParam, lParam);
	if (iLockedTheMutex) {
		orig_WndProcMutex.unlock();
		orig_WndProcMutexLockedByWndProc = false;
	}
	detouring.markHookRunning("WndProc", false);
	--detouring.hooksCounter;
	return result;
}

void EndScene::drawTrainingHudHook(char* thisArg) {
	if (gifMode.gifModeToggleHudOnly || gifMode.gifModeOn) return;
	drawTexts();
	{
		std::unique_lock<std::mutex> guard(orig_drawTrainingHudMutex);
		orig_drawTrainingHud(thisArg);
	}
}

void EndScene::drawTexts() {
	return;
	#if 0
	// Example code
	{
		char HelloWorld[] = "Hello World";
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
	    s.field167_0x120 = 0xff000000;
	    s.flags2 = 0xff000000;
	    s.x = 100;
	    s.y = 185.0 + 34 * 3;
	    s.alignment = ALIGN_LEFT;
	    s.field10_0x28 = HelloWorld;
	    s.field156_0xf4 = 0x210;
	    drawTextWithIcons(&s,0x0,1,4,0,0);
	}
    
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
	    s.field167_0x120 = 0xff000000;
	    s.flags2 = 0xff000000;
	    s.x = 460;
	    s.y = 185.0 + 34 * 3;
	    s.alignment = ALIGN_CENTER;
	    s.field10_0x28 = HelloWorld;
	    s.field156_0xf4 = 0x210;
	    drawTextWithIcons(&s,0x0,1,4,0,0);
	}
	#endif
}

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
	memset(tensionGainMaxCombo, 0, sizeof tensionGainMaxCombo);
	memset(burstGainMaxCombo, 0, sizeof burstGainMaxCombo);
}

Entity EndScene::getSuperflashInstigator() {
	if (!superflashInstigatorOffset) return nullptr;
	return Entity{ *(char**)(*aswEngine + superflashInstigatorOffset) };
}

int EndScene::getSuperflashCounterAll() {
	if (!superflashCounterAllOffset) return 0;
	return *(int*)(*aswEngine + superflashCounterAllOffset);
}

void EndScene::restartMeasuringFrameAdvantage(int index) {
	PlayerInfo& player = graphics.drawDataPrepared.players[index];
	PlayerInfo& other = graphics.drawDataPrepared.players[1 - index];
	player.frameAdvantageValid = false;
	other.frameAdvantageValid = false;
	if (other.idlePlus) {
		player.frameAdvantage = -other.timePassed;
		other.frameAdvantage = other.timePassed;
	}
	measuringFrameAdvantage = true;
}

void EndScene::restartMeasuringLandingFrameAdvantage(int index) {
	PlayerInfo& player = graphics.drawDataPrepared.players[index];
	PlayerInfo& other = graphics.drawDataPrepared.players[1 - index];
	player.landingFrameAdvantageValid = false;
	other.landingFrameAdvantageValid = false;
	if (other.idleLanding) {
		player.landingFrameAdvantage = -other.timePassedLanding;
		other.landingFrameAdvantage = other.timePassedLanding;
	}
	measuringLandingFrameAdvantage = 1 - index;
}

void EndScene::onHitDetectionStart() {
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
}

void EndScene::onHitDetectionEnd() {
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

void EndScene::onUWorld_TickBegin() {
	drawDataPrepared = false;
}

void EndScene::onUWorld_Tick() {
	if (!drawDataPrepared) {
		ui.drawData = nullptr;
		ui.needInitFont = false;
		ui.prepareDrawData();
	}
}

void EndScene::HookHelp::endSceneCallerHook(int param1, int param2, int param3) {
	++detouring.hooksCounter;
	detouring.markHookRunning("endSceneCaller", true);
	IDirect3DDevice9* device = *(IDirect3DDevice9**)((char*)this + 0x24);
	endScene.endSceneHook(device);
	{
		std::unique_lock<std::mutex> guard(endScene.orig_endSceneCallerMutex);
		endScene.orig_endSceneCaller((void*)this, param1, param2, param3);
	}
	{
		std::unique_lock<std::mutex> guard(graphics.drawDataPreparedMutex);
		graphics.needNewDrawData = true;
	}
	{
		std::unique_lock<std::mutex> guard(camera.valuesPrepareMutex);
		graphics.needNewCameraData = true;
	}
	detouring.markHookRunning("endSceneCaller", false);
	--detouring.hooksCounter;
}
