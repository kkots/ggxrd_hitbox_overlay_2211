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
#include "WError.h"
#include "UI.h"
#include "CustomWindowMessages.h"
#include "Hud.h"
#include "Moves.h"
#include "findMoveByName.h"
#include <mutex>
#include "InputsDrawing.h"
#include "InputNames.h"
#include "SpecificFramebarIds.h"
#include <chrono>
#include "NamePairManager.h"
#include <unordered_map>
#include "pi.h"
#include "ReadWholeFile.h"
#include "EndSceneRepeatingStuff.h"

static int __cdecl LifeTimeCounterCompare(void const* p1Ptr, void const* p2Ptr) {
	const Entity* p1Ent = (const Entity*)p1Ptr;
	const Entity* p2Ent = (const Entity*)p2Ptr;
	return p2Ent->lifeTimeCounter() - p1Ent->lifeTimeCounter();
}

// this function can't run while the pause menu is open in single player modes
void EndScene::logicInside() {
	if (!gifMode.modDisabled) {
		bool isPauseMenu, needToClearHitDetection;
		bool isNormalMode = altModes.isGameInNormalMode(&needToClearHitDetection, &isPauseMenu) || isPauseMenu;
		
		if (game.getGameMode() == GAME_MODE_NETWORK && game.getPlayerSide() != 2) {
			for (PlayerFramebars& elem : playerFramebars) {
				elem.advanceStateHead();
			}
			
			DWORD currentTick = getAswEngineTick();
			// scrubbing a timeline does not eradicate non-existent framebars, but advancing a tick does
			for (auto it = projectileFramebars.begin(); it != projectileFramebars.end(); ) {
				ThreadUnsafeSharedPtr<ProjectileFramebar>& elem = *it;
				if (currentTick < elem->creationTick || currentTick >= elem->deletionTick && currentTick - elem->deletionTick + 1 >= ROLLBACK_MAX) {
					it = projectileFramebars.erase(it);
					continue;
				}
				elem->advanceStateHead();
				++it;
			}
		} else {
			DWORD currentTick = getAswEngineTick();
			// scrubbing a timeline does not eradicate non-existent framebars, but advancing a tick does
			for (auto it = projectileFramebars.begin(); it != projectileFramebars.end(); ) {
				ThreadUnsafeSharedPtr<ProjectileFramebar>& elem = *it;
				if (currentTick < elem->creationTick || currentTick >= elem->deletionTick) {
					it = projectileFramebars.erase(it);
					continue;
				}
				++it;
			}
		}
		
		bool isRunning = EndScene::isRunning();
		entityList.populate();
		if (!isRunning) {
			for (int i = 0; i < 2; ++i) {
				currentState->punishMessageTimers[i].clear();
				currentState->attackerInRecoveryAfterBlock[i] = false;
			}
			memset(currentState->attackerInRecoveryAfterCreatingProjectile, 0, sizeof currentState->attackerInRecoveryAfterCreatingProjectile);
			
			if (!settings.dontClearFramebarOnStageReset) {
				playerFramebars.clear();
				projectileFramebars.clear();
				combinedFramebars.clear();
			}
			
			bool oneIsDead = currentState->lastRoundendContainedADeath;
			if (!oneIsDead) {
				for (int i = 0; i < 2; ++i) {
					if (entityList.slots[i].hp() == 0) {
						oneIsDead = true;
						break;
					}
				}
			}
			if (oneIsDead) currentState->lastRoundendContainedADeath = true;
			
			if (!currentState->lastRoundendContainedADeath) {
				// position reset
				
				if (settings.comboRecipe_clearOnPositionReset) {
					for (int i = 0; i < 2; ++i) {
						PlayerInfo& player = currentState->players[i];
						player.comboRecipe.clear();
					}
				}
				
				for (int i = 0; i < 2; ++i) {
					PlayerInfo& player = currentState->players[i];
					player.stunCombo = 0;
					player.tensionGainLastCombo = 0;
					player.burstGainLastCombo = 0;
				}
				
			}
			currentState->startedNewRound = true;
			
			for (int i = 0; i < 2; ++i) {
				PlayerInfo& player = currentState->players[i];
				player.inputs.clear();
				player.prevInput = Input{0x0000};
				player.inputsOverflow = false;
				player.ramlethalForpeliMarteliDisabled = false;
			}
		}
		if (!gifMode.gifModeToggleHudOnly && !gifMode.gifModeOn) {
			for (int i = 0; i < 2; ++i) {
				std::vector<EndSceneStoredState::PunishMessageTimer>& vec = currentState->punishMessageTimers[i];
				for (auto it = vec.begin(); it != vec.end(); ) {
					EndSceneStoredState::PunishMessageTimer& timer = *it;
					if (timer.animFrame >= 0 && timer.animFrame < (int)_countof(punishAnim)) {
						const int swipeDur = 16;
						const float swipeDistY = 30.F;
						const float swipeDistX = 10.F;
						if (timer.animFrame < swipeDur) {
							for (auto itNext = vec.begin(); itNext != it; ++itNext) {
								EndSceneStoredState::PunishMessageTimer& timerNext = *itNext;
								timerNext.yOff -= swipeDistY / (float)swipeDur;
								timerNext.xOff -= swipeDistX / (float)swipeDur;
							}
						}
						timer.currentAnimFrame = timer.animFrame;
						if (timer.animFrameRepeatCount > 0) {
							--timer.animFrameRepeatCount;
						} else {
							++timer.animFrame;
							if (timer.animFrame < (int)_countof(punishAnim)) {
								timer.animFrameRepeatCount = punishAnim[timer.animFrame].repeatCount;
							}
						}
						++it;
					} else {
						it = vec.erase(it);
					}
				}
			}
		}
		if (isNormalMode && isRunning && entityList.areAnimationsNormal()) {
			prepareDrawDataInside();
		}
	}
}

void EndScene::prepareDrawDataInside() {
	logOnce(fputs("prepareDrawData called\n", logfile));
	EndSceneStoredState* cs = currentState;
	cs->lastRoundendContainedADeath = false;
	
	bool isTheFirstFrameInTheRound = false;
	if (playerFramebars.empty()) {
		isTheFirstFrameInTheRound = true;
		
		// adding players one by one resizes the vector and causes stateHead to point to garbage. Add both and then assign stateHead
		playerFramebars.resize(2);
		for (int i = 0; i < 2; ++i) {
			PlayerFramebars& framebar = playerFramebars[i];
			framebar.playerIndex = i;
			framebar.main.stateHead = framebar.main.states;
			framebar.hitstop.stateHead = framebar.hitstop.states;
			framebar.idle.stateHead = framebar.idle.states;
			framebar.idleHitstop.stateHead = framebar.idleHitstop.states;
		}
		
		cs->framebarPosition = _countof(PlayerFramebar::frames) - 1;
		cs->framebarTotalFramesUnlimited = 0;
		cs->framebarPositionHitstop = _countof(PlayerFramebar::frames) - 1;
		cs->framebarTotalFramesHitstopUnlimited = 0;
		cs->framebarIdleFor = 0;
		cs->framebarIdleHitstopFor = 0;
		ui.onFramebarReset();
	}
	if (cs->startedNewRound) {
		cs->startedNewRound = false;
		isTheFirstFrameInTheRound = true;
		if (gifMode.editHitboxes) {
			ui.stopHitboxEditMode();
		}
	}
	if (isTheFirstFrameInTheRound) {
		bool isTraining = game.isTrainingMode();
		if (isTraining) {
			for (int i = 0; i < 2; ++i) {
				int val = i == 0 ? settings.startingTensionPulseP1 : settings.startingTensionPulseP2;
				if (val) {
					entityList.slots[i].tensionPulse() = minmax(-25000, 25000, val);
				}
			}
		}
		if (isTraining && settings.clearInputHistoryOnStageResetInTrainingMode
				|| settings.clearInputHistoryOnStageReset) {
			game.clearInputHistory();
			clearInputHistory();
		}
		for (int i = 0; i < 2; ++i) {
			// on the first frame of a round people can't act. At all
			// without this fix, the mod thinks normals are enabled except on the very first frame of a match where it thinks they're not
			PlayerInfo& player = cs->players[i];
			player.wasEnableNormals = false;
			player.wasPrevFrameEnableNormals = false;
			player.wasPrevFrameEnableWhiffCancels = false;
		}
	}
	bool framebarAdvanced = false;
	bool framebarAdvancedHitstop = false;
	bool framebarAdvancedIdle = false;
	bool framebarAdvancedIdleHitstop = false;
	
	logOnce(fprintf(logfile, "entity count: %d\n", entityList.count));

	DWORD aswEngineTickCount = getAswEngineTick();
	bool frameHasChanged = cs->prevAswEngineTickCount != aswEngineTickCount && !game.isRoundend();
	cs->prevAswEngineTickCount = aswEngineTickCount;
	
	bool needCatchEntities = false;
	if (frameHasChanged) {
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			if (!player.pawn) {
				player.index = i;
				initializePawn(player, entityList.slots[i]);
				needCatchEntities = true;
				cs->projectiles.clear();
			}
			player.wasCancels.deleteThatWhichWasNotFound();
		}
	}
	if (frameHasChanged) {
		if (needCatchEntities && cs->projectiles.empty()) {
			// probably the mod was loaded in mid-game
			for (int i = 2; i < entityList.count; ++i) {
				onObjectCreated(entityList.slots[i], entityList.list[i], entityList.list[i].animationName(), true, false);
			}
		}
		
		int distanceBetweenPlayers = entityList.slots[0].posX() - entityList.slots[1].posX();
		if (distanceBetweenPlayers < 0) distanceBetweenPlayers = -distanceBetweenPlayers;
		
		bool comboStarted = false;
		for (int i = 0; i < 2; ++i) {
			Entity ent = entityList.slots[i];
			PlayerInfo& player = cs->players[i];
			if (ent.inHitstun() && !player.inHitstunNowOrNextFrame) {
				comboStarted = true;
			}
			player.inHitstunNowOrNextFrame = ent.inHitstun();
			player.inHitstun = ent.inHitstunThisFrame();
		}
		
		Entity superflashInstigator = getSuperflashInstigator();
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			PlayerInfo& other = cs->players[1 - i];
			Entity ent = player.pawn;
			Entity otherEnt = other.pawn;
			player.prevHp = player.hp;
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
			player.burstGainCounter = ent.burstGainCounter();
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
			
			player.x = ent.posX();
			player.y = ent.posY();
			player.speedX = ent.speedX();
			player.speedY = player.lostSpeedYOnThisFrame ? player.speedYBeforeSpeedLost : ent.speedY();
			player.gravity = ent.gravity();
			player.comboTimer = ent.comboTimer();
			player.pushback = ent.pushback();
			
			player.prevFrameHadDangerousNonDisabledProjectiles = player.hasDangerousNonDisabledProjectiles;
			player.hasDangerousNonDisabledProjectiles = false;
			player.createdProjectiles.clear();
			
			bool needResetStun = false;
			if (comboStarted) {
				if (cs->tensionGainOnLastHitUpdated[i]) {
					player.tensionGainLastCombo = cs->tensionGainOnLastHit[i];
					tensionGain = player.tension - cs->tensionRecordedHit[i];
				} else {
					player.tensionGainLastCombo = 0;
				}
				if (cs->burstGainOnLastHitUpdated[i]) {
					player.burstGainLastCombo = cs->burstGainOnLastHit[i];
					burstGain = player.burst - cs->burstRecordedHit[i];
				} else {
					player.burstGainLastCombo = 0;
				}
				if (player.inHitstunNowOrNextFrame) {
					needResetStun = true;
				}
			}
			// I'm checking both players because comboStarted means either player in being combo'd right now,
			// and I just want to check if a combo is going on right now
			if (player.inHitstunNowOrNextFrame || otherEnt.inHitstun()) {
				player.tensionGainLastCombo += tensionGain;
				player.burstGainLastCombo += burstGain;
			}
			if (player.tensionGainLastCombo > player.tensionGainMaxCombo) {
				player.tensionGainMaxCombo = player.tensionGainLastCombo;
			}
			if (player.burstGainLastCombo > player.burstGainMaxCombo) {
				player.burstGainMaxCombo = player.burstGainLastCombo;
			}
			player.tensionPulsePenaltySeverity = entityManager.calculateTensionPulsePenaltySeverity(player.tensionPulsePenalty);
			player.cornerPenaltySeverity = entityManager.calculateCornerPenaltySeverity(player.cornerPenalty);
			
			if (cs->tensionGainOnLastHitUpdated[i]) {
				player.tensionGainOnLastHit = cs->tensionGainOnLastHit[i];
			}
			if (cs->burstGainOnLastHitUpdated[i]) {
				player.burstGainOnLastHit = cs->burstGainOnLastHit[i];
			}
			cs->tensionGainOnLastHitUpdated[i] = false;
			cs->burstGainOnLastHitUpdated[i] = false;
			
			if (player.inHitstunNowOrNextFrame) {
				int newStun;
				if (cs->reachedMaxStun[i] != -1) {
					newStun = cs->reachedMaxStun[i];
				} else {
					newStun = ent.stun();
				}
				if (newStun != -1 && (needResetStun || newStun > player.stunCombo)) {
					player.stunCombo = newStun;
				}
			}
			
			player.stun = ent.stun();
			player.stunThreshold = ent.stunThreshold();
			int prevFrameBlockstun = player.blockstun;
			player.blockstun = ent.blockstun();
			int prevFrameHitstun = player.hitstun;
			player.hitstun = ent.hitstun();
			player.tumble = 0;
			player.displayTumble = false;
			player.wallstick = 0;
			player.displayWallstick = false;
			player.knockdown = 0;
			player.displayKnockdown = false;
			CmnActIndex cmnActIndex = ent.cmnActIndex();
			if (cmnActIndex == CmnActBDownLoop
					|| cmnActIndex == CmnActFDownLoop
					|| cmnActIndex == CmnActVDownLoop
					|| cmnActIndex == CmnActBDown2Stand
					|| cmnActIndex == CmnActFDown2Stand
					|| cmnActIndex == CmnActWallHaritsukiLand
					|| cmnActIndex == CmnActWallHaritsukiGetUp
					|| cmnActIndex == CmnActKizetsu
					|| cmnActIndex == CmnActHajikareStand
					|| cmnActIndex == CmnActHajikareCrouch
					|| cmnActIndex == CmnActHajikareAir) {
				player.hitstun = 0;
			}
			if (cmnActIndex == CmnActKorogari) {
				if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
					player.tumbleContaminatedByRCSlowdown = false;
					player.tumbleMax = ent.received()->tumbleDuration + 30 - 1;
					player.tumbleStartedInHitstop = ent.hitstop() > 0;
					player.tumbleElapsed = 0;
				}
				player.displayTumble = true;
				if (ent.bbscrvar() == 0) {
					int animDur = ent.currentAnimDuration();
					player.tumble = ent.received()->tumbleDuration - (animDur == 1 ? 1 : player.tumbleStartedInHitstop ? animDur - 1 : animDur) + 30
						+ (!player.tumbleStartedInHitstop ? 1 : 0);
				} else {
					player.tumble = 30 - ent.bbscrvar2() + 1;
				}
			} else if (cmnActIndex == CmnActWallHaritsuki) {
				if (ent.bbscrvar2() == 0) {
					if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
						player.wallstickContaminatedByRCSlowdown = false;
						player.wallstickMax = ent.received()->wallstickDuration + 30 - 1;
						player.wallstickElapsed = 0;
					}
					player.displayWallstick = true;
					player.wallstick = ent.received()->wallstickDuration + 1 - ent.bbscrvar();
				} else {
					player.wallstick = 0;
				}
			} else if (cmnActIndex == CmnActFDownLoop
					|| cmnActIndex == CmnActBDownLoop
					|| cmnActIndex == CmnActVDownLoop) {
				int knockdownDur;
				if (ent.hp() * 10000 / 420 > 0) {
					knockdownDur = 11;
					if (ent.infiniteKd()) {
						knockdownDur = -1;
					}
					int customKd = ent.received()->knockdownDuration;
					if (customKd != 0) {
						knockdownDur = customKd;
					}
					int frame = ent.currentAnimDuration();
					if (knockdownDur != -1) {
						if (ent.currentAnimDuration() == 1 && !ent.isRCFrozen()) {
							player.knockdownContaminatedByRCSlowdown = false;
							player.knockdownMax = knockdownDur;
							player.knockdownElapsed = 0;
						}
						player.displayKnockdown = true;
						player.knockdown = knockdownDur + 1 - frame;
					} else {
						player.knockdown = 0;
					}
				} else {
					player.knockdown = 0;
				}
			}
			if (player.hitstun == prevFrameHitstun + 9 && (
					cmnActIndex == CmnActBDownBound
					|| cmnActIndex == CmnActFDownBound
					|| cmnActIndex == CmnActVDownBound)) {
				player.hitstunMaxFloorbounceExtra += 10;  // some code in CmnActFDownBound, CmnActBDownBound and CmnActVDownBound increments hitstun by 10
				                                          // try this with Sol Kudakero airhit floorbounce. Potemkin ICPM ground hit may also do this
				                                          // Greed Sever airhit does this
			}
			int hitstop = ent.hitstop();
			int clashHitstop = ent.clashHitstop();
			if (!player.clashHitstop && clashHitstop) {
				player.hitstopMax = 0;
				player.hitstopElapsed = 0;
			}
			if (player.clashHitstop) {
				int val = clashHitstop + hitstop;
				if (val > player.hitstopMax) {
					player.hitstopMax = val;
					player.hitstopElapsed = 0;
				}
			}
			player.clashHitstop = clashHitstop;
			if (ent.hitSomethingOnThisFrame()) {
				player.hitstop = 0;
				player.hitstopMax = hitstop + clashHitstop - 1;
				if (player.hitstopMax < 0) player.hitstopMax = 0;
				player.hitstopElapsed = 0;
			} else if (player.armoredHitOnThisFrame && !player.gotHitOnThisFrame && player.setHitstopMaxSuperArmor) {
				player.hitstopMax = player.hitstopMaxSuperArmor - 1;
				player.hitstopElapsed = 0;
			} else {
				player.hitstop = hitstop + clashHitstop;
				if (!player.hitstop) {
					player.hitstop = player.lastHitstopBeforeWipe;
				}
			}
			if (player.blockstun && !player.hitstop && !superflashInstigator) {
				++player.blockstunElapsed;
				if (player.rcSlowedDown) player.blockstunContaminatedByRCSlowdown = true;
			}
			
			bool jumpInstalledStage2 = player.jumpInstalledStage2;
			bool superJumpInstalledStage2 = player.superJumpInstalledStage2;
			player.jumpInstalledStage2 = false;
			player.superJumpInstalledStage2 = false;
			
			const bool isSemuke = player.charType == CHARACTER_TYPE_LEO
					&& strcmp(ent.animationName(), "Semuke") == 0;
			if (isSemuke) {
				player.wasCancels.clearDelays();
			}
			
			bool startedRunOrWalkOnThisFrame = false;
			if (player.changedAnimOnThisFrame
					|| isSemuke && ent.gotoLabelRequests()[0] != '\0') {
				
				bool oldIsRunning = player.isRunning;
				bool oldIsWalkingForward = player.isWalkingForward;
				bool oldIsWalkingBackward = player.isWalkingBackward;
				
				player.isRunning = false;
				player.isWalkingForward = false;
				player.isWalkingBackward = false;
				if (player.charType == CHARACTER_TYPE_LEO) {
					Moves::SemukeSubanim subanim = moves.parseSemukeSubanimWithCheck(player.pawn, Moves::SEMUKEPARSE_INPUT);
					switch (subanim) {
						case Moves::SEMUKESUBANIM_WALK_FORWARD: player.startedWalkingForward = true; break;
						case Moves::SEMUKESUBANIM_WALK_BACK: player.startedWalkingBackward = true; break;
					}
				}
				if (player.startedRunning) {
					player.isRunning = true;
				} else if (player.startedWalkingForward) {
					player.isWalkingForward = true;
				} else if (player.startedWalkingBackward) {
					player.isWalkingBackward = true;
				}
					
				if ((
						player.startedRunning && !oldIsRunning
						|| player.startedWalkingForward && !oldIsWalkingForward  // for combining Faust 3 and 6 walks
						|| player.startedWalkingBackward && !oldIsWalkingBackward
					) && otherEnt.inHitstun()
				) {
					startedRunOrWalkOnThisFrame = true;
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					newComboElem.name = &emptyNamePair;
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.dashDuration = 1;
					if (player.startedRunning) {
						// do nothing
					} else if (player.startedWalkingForward) {
						newComboElem.isWalkForward = true;
					} else if (player.startedWalkingBackward) {
						newComboElem.isWalkBackward = true;
					}
					
					PlayerInfo::CancelDelay cancelDelay;
					player.determineCancelDelay(&cancelDelay);
					newComboElem.cancelDelayedBy = cancelDelay.delay;
					newComboElem.doneAfterIdle = cancelDelay.isAfterIdle;
				}
			}
			
			if ((player.isRunning || player.isWalkingForward || player.isWalkingBackward)
					&& !player.hitstop && !superflashInstigator && !startedRunOrWalkOnThisFrame && otherEnt.inHitstun()) {
				ComboRecipeElement* lastElem = player.findLastDash();
				if (lastElem) {
					bool ok = false;
					if (player.isRunning) {
						ok = !lastElem->isWalkForward && !lastElem->isWalkBackward;
					} else if (player.isWalkingForward) {
						ok = lastElem->isWalkForward;
					} else if (player.isWalkingBackward) {
						ok = lastElem->isWalkBackward;
					}
					if (ok) {
						++lastElem->dashDuration;
					}
				}
			}
			
			player.wallslumpLand = 0;
			player.wallslumpLandWithSlow = 0;
			player.rejection = 0;
			player.rejectionWithSlow = 0;
			
			const CmnActIndex entCmnActIndex = ent.cmnActIndex();
			if (entCmnActIndex == CmnActJitabataLoop) {
				int bbscrvar = ent.bbscrvar();
				if (player.changedAnimOnThisFrame) {
					player.staggerElapsed = 0;
					player.staggerMaxFixed = false;
					player.prevBbscrvar = 0;
					player.prevBbscrvar5 = ent.receivedAttack()->staggerDuration * 10;
				}
				if (!player.hitstop && !superflashInstigator) ++player.staggerElapsed;
				int staggerMax = player.prevBbscrvar5 / 10 + ent.thisIsMinusOneIfEnteredHitstunWithoutHitstop();
				int animDur = ent.currentAnimDuration();
				if (!bbscrvar) {
					player.staggerMax = staggerMax;
				} else if (!player.prevBbscrvar) {
					player.staggerMaxFixed = true;
					if (staggerMax - 4 < animDur) {
						staggerMax = animDur + 4;
					}
					player.staggerMax = staggerMax;
				}
				player.stagger = player.staggerMax - (animDur - 1) - (player.hitstop ? 1 : 0);
				player.prevBbscrvar = bbscrvar;
			} else if (entCmnActIndex == CmnActHajikareStand
					|| entCmnActIndex == CmnActHajikareCrouch
					|| entCmnActIndex == CmnActHajikareAir) {
				int animDur = ent.currentAnimDuration();
				if (animDur == 1 && !ent.isSuperFrozen() && !ent.isRCFrozen()) {
					player.rejectionElapsed = 0;
				}
				if (!superflashInstigator && !player.hitstop) {
					++player.rejectionElapsed;
				}
				int hitEffect = ent.currentHitEffect();
				if (hitEffect >= 22 && hitEffect <= 25) {
					player.rejectionMax = 30;
					player.rejection = 31 - animDur;
				} else {
					player.rejectionMax = 60;
					player.rejection = 61 - animDur;
				}
			} else if (entCmnActIndex == CmnActWallHaritsukiLand) {
				if (player.changedAnimOnThisFrame) {
					player.wallslumpLandElapsed = 0;
				}
				if (!player.hitstop && !superflashInstigator) ++player.wallslumpLandElapsed;
				int animDur = ent.currentAnimDuration();
				player.wallslumpLand = 26 - animDur + 1;
				player.wallslumpLandMax = 26;
				player.displayHitstop = false;
			}
			
			player.airborne = player.airborne_afterTick();
			int prevFrameWakeupTiming = player.wakeupTiming;
			player.wakeupTiming = 0;
			CmnActIndex prevFrameCmnActIndex = player.cmnActIndex;
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
			if (!prevFrameWakeupTiming && player.wakeupTiming) {
				player.wakeupTimingElapsed = 0;
			}
			if (player.wakeupTiming && !superflashInstigator) ++player.wakeupTimingElapsed;
			if (player.cmnActIndex == CmnActKirimomiUpper && ent.currentAnimDuration() == 2 && !ent.isRCFrozen()) {
				player.hitstunMax = player.hitstun;  // 5D6 hitstun
				player.hitstunElapsed = 0;
				player.hitstunContaminatedByRCSlowdown = false;
				player.hitstunMaxFloorbounceExtra = 0;
			}
			player.inBlockstunNextFrame = ent.inBlockstunNextFrame();
			if (player.inBlockstunNextFrame) {
				player.lastBlockWasFD = isHoldingFD(ent);
				player.lastBlockWasIB = !player.lastBlockWasFD && ent.successfulIB();
			}
			const char* animName = ent.animationName();
			player.onTheDefensive = player.blockstun
				|| player.inHitstun
				|| cmnActIndex == CmnActUkemi
				|| ent.gettingUp();
			if (!player.obtainedForceDisableFlags) {
				player.wasForceDisableFlags = ent.forceDisableFlags();
			}
			player.enableBlock = ent.enableBlock();
			bool prevFrameCanFaultlessDefense = player.canFaultlessDefense;
			// I gave up trying to understand how moves work. Hardcode/make shit up time!
			player.canFaultlessDefense = (player.enableBlock || player.wasEnableNormals || whiffCancelIntoFDEnabled(player))
				&& ent.fdNegativeCheck() == 0
				&& player.wasProhibitFDTimer == 0
				&& ent.dizzyMashAmountLeft() <= 0
				&& ent.exKizetsu() <= 0
				|| player.move.canFaultlessDefend && player.move.canFaultlessDefend(player);
			player.fillInMove();
			bool idleNext = player.move.isIdle(player);
			if (player.airborne && player.move.forceLandingRecovery) player.moveOriginatedInTheAir = true;
			bool prevFrameIdle = player.idle;
			bool prevFrameIgnoreNextInabilityToBlockOrAttack = player.ignoreNextInabilityToBlockOrAttack;
			bool prevFrameCanBlock = player.canBlock;
			player.canBlock = player.move.canBlock(player);
			
			#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) player.fieldName.active = false;
			INVUL_TYPES_TABLE
			#undef INVUL_TYPES_EXEC
			
			player.projectileOnlyInvul.active = !ent.strikeInvul()
				&& ent.strikeInvulnFrames() <= 0
				&& !player.wasFullInvul
				&& (
					player.wasSuperArmorEnabled
					&& ent.superArmorType() == SUPER_ARMOR_DODGE
					&& !ent.superArmorHontaiAttacck()
					&& !ent.superArmorForReflect()
				);
			player.reflect.active = !player.wasFullInvul
				&& (
					(
						player.wasSuperArmorEnabled
						|| ent.strikeInvul()
						|| ent.strikeInvulnFrames() > 0
					)
					&& ent.superArmorType() == SUPER_ARMOR_DODGE
					&& ent.superArmorForReflect()
				);
			
			player.counterhit = false;
			player.crouching = false;
			
			if (player.changedAnimOnThisFrame) {
				player.maxHit.clear();
				player.hitNumber = 0;
			}
			
			if (!player.leftBlockstunHitstun
					&& !player.receivedNewDmgCalcOnThisFrame
					&& (
						player.cmnActIndex == -1
						|| player.wakeupTiming
						|| !player.inHitstun
						&& !player.blockstun
						&& !player.pawn.blockCount()
						&& player.cmnActIndex != CmnActAirGuardLoop
						&& player.cmnActIndex != CmnActAirGuardEnd
						&& player.cmnActIndex != CmnActCrouchGuardLoop
						&& player.cmnActIndex != CmnActCrouchGuardEnd
						&& player.cmnActIndex != CmnActHighGuardLoop
						&& player.cmnActIndex != CmnActHighGuardEnd
						&& player.cmnActIndex != CmnActMidGuardLoop
						&& player.cmnActIndex != CmnActMidGuardEnd
					)) {
				player.leftBlockstunHitstun = true;
			}
			player.proration = ent.proration();
			player.dustProration1 = ent.dustProration1();
			player.dustProration2 = ent.dustProration2();
			player.comboProrationNormal = entityManager.calculateComboProration(ent.risc(), ATTACK_TYPE_NORMAL);
			player.comboProrationOverdrive = entityManager.calculateComboProration(ent.risc(), ATTACK_TYPE_OVERDRIVE);
			player.rcProration = other.pawn.rcSlowdownCounter() != 0 && !other.pawn.ignoreRcProration();
			
			if (!player.changedAnimOnThisFrame
					&& prevFrameIdle
					// Leaving Rifle or Pogo stance after spending too much time in it
					&& player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
					&& !idleNext
					&& player.charType != CHARACTER_TYPE_LEO
				) {
				player.changedAnimOnThisFrame = true;
			}
			player.forceBusy = false;
			if (player.charType == CHARACTER_TYPE_LEO
					&& strcmp(animName, "Semuke") == 0) {
				bool isSemukeEnd = strcmp(ent.gotoLabelRequests(), "SemukeEnd") == 0;
				if (isSemukeEnd) {
					player.changedAnimOnThisFrame = true;
				}
				if (isSemukeEnd || !ent.enableWhiffCancels()) {
					player.canBlock = false;
					player.forceBusy = true;  // if I just set idleNext to false here, the framebar will display Leo as busy on that frame, but that's not true - he can act
				}
			}
			if (player.changedAnimOnThisFrame
					&& (
						prevFrameCmnActIndex == CmnActBDownUpper
						&& player.cmnActIndex == CmnActBDownDown
						|| prevFrameCmnActIndex == CmnActFDownUpper
						&& player.cmnActIndex == CmnActFDownDown
						|| prevFrameCmnActIndex == CmnActVDownUpper
						&& player.cmnActIndex == CmnActVDownDown
						|| prevFrameCmnActIndex == CmnActJump
						&& player.cmnActIndex == CmnActJump
						&& ent.remainingDoubleJumps() != player.remainingDoubleJumps - 1
						|| !prevFrameBlockstun  // proximity blocking - for displaying gaps in framebar
						&& !player.blockstun
						&& idleNext  // exclude voluntary FD
						&& (
							(
								prevFrameCmnActIndex == CmnActMidGuardEnd
								|| prevFrameCmnActIndex == CmnActCrouchGuardEnd
								|| prevFrameCmnActIndex == CmnActHighGuardEnd
								|| prevFrameCmnActIndex == CmnActAirGuardEnd
								|| prevFrameCmnActIndex == CmnActMidGuardLoop
								|| prevFrameCmnActIndex == CmnActCrouchGuardLoop
								|| prevFrameCmnActIndex == CmnActHighGuardLoop
								|| prevFrameCmnActIndex == CmnActAirGuardLoop
							)
							|| prevFrameIdle
							&& (
								cmnActIndex == CmnActMidGuardEnd
								|| cmnActIndex == CmnActCrouchGuardEnd
								|| cmnActIndex == CmnActHighGuardEnd
								|| cmnActIndex == CmnActAirGuardEnd
								|| cmnActIndex == CmnActMidGuardLoop
								|| cmnActIndex == CmnActCrouchGuardLoop
								|| cmnActIndex == CmnActHighGuardLoop
								|| cmnActIndex == CmnActAirGuardLoop
							)
						)
						// for clearer display of frame advantage in the framebar, but show turning around and crouching
						|| prevFrameIdle
						&& idleNext
						&& (
							prevFrameCmnActIndex == CmnActStand2Crouch
							&& cmnActIndex == CmnActCrouch
							|| prevFrameCmnActIndex == CmnActCrouch2Stand
							&& cmnActIndex == CmnActStand
							|| prevFrameCmnActIndex == CmnActAirTurn
							&& cmnActIndex == CmnActJump
							|| prevFrameCmnActIndex == CmnActStandTurn
							&& cmnActIndex == CmnActStand
							|| prevFrameCmnActIndex == CmnActCrouchTurn
							&& cmnActIndex == CmnActCrouch
						)
					)) {
				player.changedAnimOnThisFrame = false;
			}
			int remainingDoubleJumps = ent.remainingDoubleJumps();
			int remainingAirDashes = ent.remainingAirDashes();
			if ((remainingDoubleJumps > player.remainingDoubleJumps
					|| remainingAirDashes > player.remainingAirDashes)
					&& player.airborne) {
				player.regainedAirOptions = true;
			}
			player.remainingDoubleJumps = ent.remainingDoubleJumps();
			player.remainingAirDashes = ent.remainingAirDashes();
			
			// for CmnActBDownUpper changing to CmnActBDownDown losing hitstop display. Try Sol j.D Kudakero airborne hit
			bool dontResetHitstopMax = player.changedAnimOnThisFrame
					&& player.onTheDefensive
					&& !(player.setBlockstunMax || player.setHitstunMax)
					&& !player.wakeupTiming;
			
			player.isLanding = cmnActIndex == CmnActJumpLanding
				|| cmnActIndex == CmnActLandingStiff
				|| player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation
				&& !player.airborne
				&& !idleNext
				&& !player.onTheDefensive;
			player.isLandingOrPreJump = player.isLanding || cmnActIndex == CmnActJumpPre;
			if (cmnActIndex == CmnActJumpPre || player.airborne && !player.idle && !player.idleInNewSection) {  // second check needed for Venom Ball teleport
				player.frameAdvantageValid = false;
				other.frameAdvantageValid = false;
				player.landingFrameAdvantageValid = false;
				other.landingFrameAdvantageValid = false;
				player.frameAdvantageIncludesIdlenessInNewSection = false;
				other.frameAdvantageIncludesIdlenessInNewSection = false;
				other.eddie.frameAdvantageIncludesIdlenessInNewSection = false;
				player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = false;
			}
			memcpy(player.prevAttackLockAction, player.attackLockAction, 32);
			memcpy(player.attackLockAction, ent.dealtAttack()->attackLockAction, 32);
			player.animFrame = ent.currentAnimDuration();
			player.hitboxesCount = 0;
			
			player.dustGatlingTimer = player.pawn.dustGatlingTimer();
			if (!player.dustGatlingTimer) player.dustGatlingTimer = player.pawn.dustPropulsion();
			if (!player.dustGatlingTimer) player.dustGatlingTimerMax = 0;
			if (player.dustGatlingTimer > player.dustGatlingTimerMax) player.dustGatlingTimerMax = player.dustGatlingTimer;
			
			if (idleNext != player.idle || player.wasIdle && !idleNext || (player.setBlockstunMax || player.setHitstunMax) && !idleNext) {
				player.idle = idleNext;
				if (!player.idle) {
					if (player.onTheDefensive) {
						player.startedDefending = true;
						if (player.cmnActIndex != CmnActUkemi && !player.baikenReturningToBlockstunAfterAzami) {
							player.hitstopMax = player.hitstop;
							player.hitstopElapsed = 0;
							if (player.setBlockstunMax && player.hitstop) {  // may enter blockstun without hitstop due to Mist Finer Bacchus Sigh-buffed airhit when it glitches out and does not become unblockable
								--player.blockstunMax;  // this is the line PlayerInfo.h:PlayerInfo::inBlockstunNextFrame refers to
							}
							if (player.setHitstunMax) {
								player.hitstunMax = ent.hitstun();
								player.hitstunElapsed = 0;
								player.hitstunContaminatedByRCSlowdown = false;
								player.hitstunMaxFloorbounceExtra = 0;
								if (ent.inHitstunNextFrame() && player.lastHitstopBeforeWipe
										|| ent.hitstop()) {
									--player.hitstunMax;
								}
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
					player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = true;
					player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation = true;
					player.landingFrameAdvantageValid = false;
					other.landingFrameAdvantageValid = false;
					player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
				}
			}
			
			if (player.hitstun && !player.hitstop && !superflashInstigator) {
				++player.hitstunElapsed;
				if (player.rcSlowedDown) player.hitstunContaminatedByRCSlowdown = true;
			}
			if (player.tumble && !player.hitstop && !superflashInstigator) {
				++player.tumbleElapsed;
				if (player.rcSlowedDown) player.tumbleContaminatedByRCSlowdown = true;
			}
			if (player.wallstick && !player.hitstop && !superflashInstigator) {
				++player.wallstickElapsed;
				if (player.rcSlowedDown) player.wallstickContaminatedByRCSlowdown = true;
			}
			if (player.knockdown && !player.hitstop && !superflashInstigator) {
				++player.knockdownElapsed;
				if (player.rcSlowedDown) player.knockdownContaminatedByRCSlowdown = true;
			}
			if (player.hitstop && !superflashInstigator) ++player.hitstopElapsed;
			int newSlow;
			int unused;
			PlayerInfo::calculateSlow(player.hitstopElapsed,
				player.hitstop,
				player.rcSlowedDownCounter,
				&player.hitstopWithSlow,
				&player.hitstopMaxWithSlow,
				&newSlow);
			PlayerInfo::calculateSlow(player.hitstunElapsed,
				player.inHitstun
					? (
						player.hitstun - (player.hitstop ? 1 : 0)
					) : 0,
				newSlow,
				&player.hitstunWithSlow,
				&player.hitstunMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.tumbleElapsed,
				player.tumble,
				newSlow,
				&player.tumbleWithSlow,
				&player.tumbleMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.wallstickElapsed,
				player.wallstick,
				newSlow,
				&player.wallstickWithSlow,
				&player.wallstickMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.knockdownElapsed,
				player.knockdown,
				newSlow,
				&player.knockdownWithSlow,
				&player.knockdownMaxWithSlow,
				&unused);
			PlayerInfo::calculateSlow(player.blockstunElapsed,
				player.blockstun - (player.hitstop ? 1 : 0),
				newSlow,
				&player.blockstunWithSlow,
				&player.blockstunMaxWithSlow,
				&unused);
			if (player.cmnActIndex == CmnActJitabataLoop) {
				PlayerInfo::calculateSlow(player.staggerElapsed,
					player.stagger,
					newSlow,
					&player.staggerWithSlow,
					&player.staggerMaxWithSlow,
					&unused);
			}
			if (player.wakeupTiming) {
				PlayerInfo::calculateSlow(player.wakeupTimingElapsed,
					player.wakeupTiming - player.animFrame + 1,
					player.rcSlowedDownCounter,
					&player.wakeupTimingWithSlow,
					&player.wakeupTimingMaxWithSlow,
					&unused);
			}
			if (player.cmnActIndex == CmnActHajikareStand
					|| player.cmnActIndex == CmnActHajikareCrouch
					|| player.cmnActIndex == CmnActHajikareAir) {
				PlayerInfo::calculateSlow(player.rejectionElapsed,
					player.rejection,
					newSlow,
					&player.rejectionWithSlow,
					&player.rejectionMaxWithSlow,
					&unused);
			}
			if (player.cmnActIndex == CmnActWallHaritsukiLand) {
				PlayerInfo::calculateSlow(player.wallslumpLandElapsed,
					player.wallslumpLand,
					newSlow,
					&player.wallslumpLandWithSlow,
					&player.wallslumpLandMaxWithSlow,
					&unused);
			}
			int playerval0 = ent.playerVal(0);
			int playerval1 = ent.playerVal(1);
			const char* playervalSetterName = nullptr;
			int playervalNum = 1;
			int maxDI = 0;
			if (player.charType == CHARACTER_TYPE_SOL) {
				playervalSetterName = "DragonInstall";
				maxDI = player.wasPlayerval1Idling;
			} else if (player.charType == CHARACTER_TYPE_CHIPP) {
				playervalSetterName = "Meisai";
				playervalNum = 0;
				maxDI = player.pawn.playerVal(0);
			} else if (player.charType == CHARACTER_TYPE_MILLIA) {
				playervalSetterName = "ChromingRose";
				maxDI = player.wasPlayerval1Idling + 10;
			} else if (player.charType == CHARACTER_TYPE_SLAYER) {
				playervalSetterName = "ChiwosuuUchuuExe";
				maxDI = player.wasPlayerval1Idling;
			}
			if (playervalSetterName && strcmp(animName, playervalSetterName) == 0) {
				player.fillInPlayervalSetter(playervalNum);
				if (player.playervalSetterOffset != -1
						&& player.pawn.bbscrCurrentInstr() - player.pawn.bbscrCurrentFunc() == player.playervalSetterOffset
						&& player.pawn.justReachedSprite()) {
					player.maxDI = maxDI;
				}
			}
			player.playerval1 = playerval1;
			player.playerval0 = playerval0;
			
			int poisonDuration = ent.poisonDuration();
			if (!player.poisonDuration) {
				player.poisonDurationMax = poisonDuration;
			} else if (poisonDuration > player.poisonDuration) {
				player.poisonDurationMax = poisonDuration;
			}
			player.poisonDuration = poisonDuration;
			
			player.maxHit.fill(ent, ent.currentHitNum());
			
			bool isHitOnThisFrame = player.pawn.inHitstunNextFrame();
			player.wasHitOnPreviousFrame = player.wasHitOnThisFrame;
			player.wasHitOnThisFrame = isHitOnThisFrame;
			
			bool clearSuperStuff = false;
			if (player.inHitstunNowOrNextFrame && !(player.wasEnableAirtech && player.hitstun == 0)) {
				if (!player.gettingHitBySuper
						&& other.move.forceSuperHitAnyway
						&& other.move.forceSuperHitAnyway(other)) {
					player.gettingHitBySuper = true;
				} else if (player.gettingHitBySuper
						&& (
							other.recovery
							&& other.move.iKnowExactlyWhenTheRecoveryOfThisMoveIs
							&& other.move.iKnowExactlyWhenTheRecoveryOfThisMoveIs(other)
							|| other.landingRecovery
						)) {
					clearSuperStuff = true;
				}
			} else {
				clearSuperStuff = true;
			}
			if (clearSuperStuff) {
				player.gettingHitBySuper = false;
				player.startedSuperWhenComboing = false;
			}
			
			player.gettingUp = ent.gettingUp();
			bool idlePlus = player.idle
				|| player.idleInNewSection
				|| player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding
				&& player.isLanding;
			if (idlePlus != player.idlePlus) {
				player.timePassed = 0;
				player.idlePlus = idlePlus;
			}
			player.sprite.fill(ent);
			player.isInFDWithoutBlockstun = (
					player.cmnActIndex == CmnActCrouchGuardLoop
					|| player.cmnActIndex == CmnActMidGuardLoop
					|| player.cmnActIndex == CmnActAirGuardLoop
					|| player.cmnActIndex == CmnActHighGuardLoop
				)
				&& player.blockstun == 0
				&& (
					!player.idle  // idle check needed because proxyguard also triggers these animations
					|| !player.canBlock  // block check also needed because 5D6 cannot be immediately FD cancelled
					&& !player.ignoreNextInabilityToBlockOrAttack  // this check must always follow the canBlock check
				);
			bool needDisableProjectiles = false;
			player.changedAnimFiltered = false;
			if (player.changedAnimOnThisFrame
					&& !(player.cmnActIndex == CmnActJump && !player.canFaultlessDefense)) {
				player.sinHunger = false;
				player.charge.current = 0;
				player.charge.max = 0;
				player.wokeUp = prevFrameWakeupTiming && !player.wakeupTiming && idleNext;
				if (!player.move.preservesNewSection) {
					player.inNewMoveSection = false;
					if (!cs->measuringFrameAdvantage) {
						player.frameAdvantageIncludesIdlenessInNewSection = true;
						other.frameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.frameAdvantageIncludesIdlenessInNewSection = true;
					}
					if (cs->measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageIncludesIdlenessInNewSection = true;
						other.landingFrameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = true;
					}
					if (player.idleInNewSection) {
						player.idleInNewSection = false;
						player.idlePlus = false;
						player.timePassed = 0;
					}
				}
				player.changedAnimFiltered = !(
					player.move.combineWithPreviousMove
					&& !player.move.usePlusSignInCombination
					&& !(
						player.charType == CHARACTER_TYPE_HAEHYUN
						&& strcmp(player.anim, "AntiAirAttack") != 0
						&& strcmp(animName, "AntiAir4Hasei") == 0
						|| player.charType == CHARACTER_TYPE_JOHNNY
						&& ent.mem54()  // when Mem(54) is set, MistFinerLoop/AirMistFinerLoop instantly transitions to a chosen Mist Finer. However, in Rev1, it takes one frame
						&& (
							strcmp(animName, "MistFinerLoop") == 0
							|| strcmp(animName, "AirMistFinerLoop") == 0
						)
					)
					|| player.charType == CHARACTER_TYPE_JAM
					&& strcmp(player.anim, "NeoHochihu") == 0  // 54625 causes NeoHochihu to cancel into itself
					&& strcmp(animName, "NeoHochihu") == 0
					&& !player.wasEnableWhiffCancels
					|| player.charType == CHARACTER_TYPE_JOHNNY
					&& ent.mem54()
					&& (
						strcmp(player.anim, "MistFinerLoop") == 0
						&& (
							strncmp(animName, "MistFinerALv", 12) == 0
							|| strncmp(animName, "MistFinerBLv", 12) == 0
							|| strncmp(animName, "MistFinerCLv", 12) == 0
						)
						|| strcmp(player.anim, "AirMistFinerLoop") == 0
						&& (
							strncmp(animName, "AirMistFinerALv", 15) == 0
							|| strncmp(animName, "AirMistFinerBLv", 15) == 0
							|| strncmp(animName, "AirMistFinerCLv", 15) == 0
						)
					)
				);
				if (!player.changedAnimFiltered) {
					if (player.charType == CHARACTER_TYPE_JOHNNY
							&& (
								strcmp(animName, "MistFinerLoop") == 0
								|| strcmp(animName, "AirMistFinerLoop") == 0
							)
					) {
						player.dontUpdateLastPerformedMoveNameInComboRecipe = true;
					}
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
					moves.forCancels = true;
					player.determineMoveNameAndSlangName(&player.lastPerformedMoveNameForComboRecipe);
					moves.forCancels = false;
				}
				if (*player.prevAttackLockAction != '\0' && strcmp(animName, player.prevAttackLockAction) == 0
						&& !player.move.dontSkipGrab) {
					player.grab = true;  // this doesn't work on regular ground and air throws
					memcpy(player.grabAnimation, player.prevAttackLockAction, 32);
				}
				if (strcmp(animName, player.grabAnimation) != 0) {
					player.grab = false;
				}
				if (player.idle && (
						player.canBlock && player.canFaultlessDefense
						|| player.wasIdle  // needed for linking a normal with a forward dash
					)) {
					player.ignoreNextInabilityToBlockOrAttack = true;
					player.performingASuper = false;
					other.gettingHitBySuper = false;
					player.sinHunger = false;
				}
				
				bool runInitNewMoveForComboRecipe = false;
				if (
						!player.changedAnimFiltered
						&& !(
							player.charType == CHARACTER_TYPE_BEDMAN
							&& strcmp(player.anim, "Boomerang_B") != 0
							&& strcmp(player.anim, "Boomerang_B_Air") != 0
							&& strcmp(animName, "BWarp") == 0
						)
						|| !player.moveNonEmpty
						&& *player.prevAttackLockAction != '\0'
						&& strcmp(animName, player.prevAttackLockAction) == 0
						|| player.isInFDWithoutBlockstun  // this check is here and not down below because I don't want air button +
						                                  // + recover in air + short FD + keep falling = to mess up the moveOriginatedInTheAir flag
						&& (
							!prevFrameIdle
							|| !prevFrameCanBlock
							&& !prevFrameIgnoreNextInabilityToBlockOrAttack
						)
						&& !player.prejumped
					) {
					if (player.move.splitForComboRecipe) {
						runInitNewMoveForComboRecipe = true;
					} else if (!player.isInFDWithoutBlockstun && player.lastPerformedMoveNameIsInComboRecipe && !player.comboRecipe.empty()) {
						ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
						if (lastElem) {
							lastElem->name = player.lastPerformedMoveNameForComboRecipe;
						}
					}
				} else {
					player.moveOriginatedInTheAir = false;
					player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation = false;  // this assignment is here and not down below,
					            // because it's for custom landing animations that are part of moves like May Hop on Dolphin or CounterGuardAir (Air Blitz whiff),
					            // and those animations don't change to another animation all the way until the landing recovery is over, so I want to know
					            // that the animation changed to whatever other one and be sure that the player being busy on the ground now is not
					            // part of a custom landing animation 100%.
					            // This assignment is not higher above because some animations I just consider an inseparable part of another animation.
					            
		            bool conditionPartOne = !(
		            			player.isLandingOrPreJump
		            			&& !(
									player.charType == CHARACTER_TYPE_SLAYER
				        			&& cmnActIndex == CmnActJumpPre
				        			&& prevFrameCmnActIndex == CmnActBDash
			        			)
		            		)
							&& player.cmnActIndex != CmnActJump
							&& (!player.idlePlus || player.forceBusy);
		            bool conditionPartTwo = !(
								!player.wasIdle
								&& player.cmnActIndex == CmnActRomanCancel
								&& player.startedUp
							);
					if (conditionPartOne && conditionPartTwo) {
						
						if (!player.baikenReturningToBlockstunAfterAzami  // technically Baiken changes animation when she puts herself into blockstun from a successful Azami with no followup,
						                                                  // and normally we stop displaying old hitstop when the animation changes, but in this case we should keep it
								&& !dontResetHitstopMax) {  // for CmnActBDownUpper changing to CmnActBDownDown losing hitstop display
							player.displayHitstop = false;
						}
						if (!player.onTheDefensive) {
							player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_NONE;
						}
						player.moveOriginatedInTheAir = player.airborne && player.wasAirborneOnAnimChange;
						
						player.startupProj = 0;
						player.hitOnFrameProj = 0;
						player.startupProjIgnoredForFramebar = 0;
						player.activesProj.clear();
						player.maxHitUse.clear();
						player.prevStartupsProj.clear();
						player.maxHitProj.clear();
						player.maxHitProjConflict = false;
						player.maxHitProjLastPtr = nullptr;
						
						if (player.charType == CHARACTER_TYPE_ELPHELT
								&& player.elpheltShotgunCharge.max) {
							if (!playerval1
									&& !player.move.charge  // may fire the shotgun when not having charge, but I still have to display the charge
									|| !playerval0
									|| player.elpheltShotgunChargeConsumed) {
								player.elpheltShotgunCharge.current = 0;
								player.elpheltShotgunCharge.max = 0;
								player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames = 0;
							}
						}
						
						if (player.move.usePlusSignInCombination
								|| player.charType == CHARACTER_TYPE_BAIKEN
								&& (
									strcmp(player.anim, "BlockingStand") == 0
									|| strcmp(player.anim, "BlockingCrouch") == 0
									|| strcmp(player.anim, "BlockingAir") == 0
								)
								&& (
									strcmp(animName, "BlockingStand") == 0
									|| strcmp(animName, "BlockingCrouch") == 0
									|| strcmp(animName, "BlockingAir") == 0
								)
								|| player.charType == CHARACTER_TYPE_ELPHELT
								&& !player.idle
								&& prevFrameCmnActIndex == CmnActRomanCancel
								&& strcmp(animName, "Rifle_Roman") == 0
								|| player.charType == CHARACTER_TYPE_JOHNNY
								&& !player.wasIdle
								&& (
									strcmp(animName, "MistFinerA") == 0
									|| strcmp(animName, "MistFinerB") == 0
									|| strcmp(animName, "MistFinerC") == 0
									|| strcmp(animName, "AirMistFinerA") == 0
									|| strcmp(animName, "AirMistFinerB") == 0
									|| strcmp(animName, "AirMistFinerC") == 0
								)
								|| player.charType == CHARACTER_TYPE_POTEMKIN
								&& !player.wasIdle
								&& (
									strcmp(animName, "HammerFall") == 0
								)
								|| !player.wasIdle
								&& player.cmnActIndex == CmnActRomanCancel
								&& !player.startedUp
								|| player.charType == CHARACTER_TYPE_SLAYER
			        			&& (
			        				player.performingBDC
			        				&& prevFrameCmnActIndex == CmnActJumpPre
			        				&& !player.airborne
			        				|| prevFrameCmnActIndex == CmnActBDash
		        				)
		        				&& !idleNext
		        				&& !player.inHitstunNowOrNextFrame) {
							if (player.superfreezeStartup) {
								player.prevStartups.add(player.superfreezeStartup,
									player.lastMoveIsPartOfStance,
									player.lastPerformedMoveName);
								player.total -= player.superfreezeStartup;  // needed for Elphelt Rifle RC
								player.superfreezeStartup = 0;
							}
							player.prevStartups.add(player.total,
								player.lastMoveIsPartOfStance,
								player.lastPerformedMoveName);
							if (player.charType == CHARACTER_TYPE_JOHNNY
									&& (
										strncmp(animName, "MistFinerALv", 12) == 0
										|| strncmp(animName, "MistFinerBLv", 12) == 0
										|| strncmp(animName, "MistFinerCLv", 12) == 0
										|| strncmp(animName, "AirMistFinerALv", 15) == 0
										|| strncmp(animName, "AirMistFinerBLv", 15) == 0
										|| strncmp(animName, "AirMistFinerCLv", 15) == 0
										|| strcmp(animName, "StepTreasureHunt") == 0
									)
							) {
								player.removeNonStancePrevStartups();
							}
						} else {
							player.onAnimReset();
						}
						
						memcpy(player.labelAtTheStartOfTheMove, player.pawn.gotoLabelRequests(), 32);
						player.airteched = player.cmnActIndex == CmnActUkemi;
						player.startup = 0;
						player.startedUp = false;
						player.actives.clear();
						player.maxHit.clear();
						player.recovery = 0;
						player.total = 0;
						player.totalForInvul = 0;
						player.stoppedMeasuringInvuls = false;
						player.lastMoveIsPartOfStance = player.move.partOfStance;
						player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
						moves.forCancels = true;
						player.determineMoveNameAndSlangName(&player.lastPerformedMoveNameForComboRecipe);
						moves.forCancels = false;
						
						if (player.charType == CHARACTER_TYPE_JOHNNY
							&& (
								strcmp(animName, "MistFinerFWalk") == 0
								|| strcmp(animName, "MistFinerBWalk") == 0
							)) {
							player.dontUpdateLastPerformedMoveNameInComboRecipe = true;
						} else {
							runInitNewMoveForComboRecipe = true;
						}
						
						player.hitOnFrame = 0;
						player.totalFD = 0;
						player.totalCanBlock = 0;
						player.totalCanFD = 0;
						player.ignoreNextInabilityToBlockOrAttack = false;
						player.performingASuper = player.pawn.dealtAttack()->type == ATTACK_TYPE_OVERDRIVE
							&& !player.move.dontSkipSuper;
						if (player.performingASuper) {
							player.startedSuperWhenComboing = other.pawn.comboCount() > 1;
						} else {
							player.startedSuperWhenComboing = false;
						}
						other.gettingHitBySuper = false;
						
						player.dontRestartTheNonLandingFrameAdvantageCountdownUponNextLanding = false;
						
						player.landingRecovery = 0;
						player.sinHungerRecovery = 0;
						player.superfreezeStartup = 0;
						
						player.hitboxTopBottomValid = false;
						player.hurtboxTopBottomValid = false;
						player.prejumped = false;
						player.performingBDC = player.charType == CHARACTER_TYPE_SLAYER
		        			&& cmnActIndex == CmnActJumpPre
		        			&& prevFrameCmnActIndex == CmnActBDash;
						
						if (player.animFrame == 1) {
							int actualMoveIndex = player.pawn.currentMoveIndex();
							if (actualMoveIndex == -1) {
								player.regainedAirOptions = false;
							} else if (!player.wasIdle && player.cmnActIndex == CmnActRomanCancel && player.airborne) {
								player.regainedAirOptions = true;
							} else {
								const AddedMoveData* actualMove = player.pawn.movesBase() + actualMoveIndex;
								if (
										actualMove->type == MOVE_TYPE_SPECIAL
										&& !(
											actualMove->characterState == MOVE_CHARACTER_STATE_JUMPING
											|| actualMove->characterState == MOVE_CHARACTER_STATE_NONE
											&& player.airborne
										)
										&& (
											player.remainingDoubleJumps != 0
											|| player.remainingAirDashes != 0
										)
								) {
									player.regainedAirOptions = true;
								} else {
									player.regainedAirOptions = false;
								}
							}
						}
						
						player.throwRangeValid = false;
						player.throwXValid = false;
						player.throwYValid = false;
						
						if (player.cmnActIndex != CmnActRomanCancel) {
							needDisableProjectiles = true;
						}
					} else if (conditionPartOne && !conditionPartTwo) {
						// Jam Hououshou final grounded leg kick RRC its startup - without this fix the frame after RRC superfreeze will be skipped
						// and the frame tooltip after will say "skipped: 1 super, 18 superfreeze, 1 super" which is very weird
						player.performingASuper = false;
						other.gettingHitBySuper = false;
						
						// and this is for combo recipe window to display RCs
						if (other.pawn.inHitstun()) {
							ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
							player.comboRecipe.emplace_back();
							ComboRecipeElement& newComboElem = player.comboRecipe.back();
							player.determineMoveNameAndSlangName(&newComboElem.name);
							newComboElem.timestamp = aswEngineTickCount;
							newComboElem.framebarId = -1;
							player.lastPerformedMoveNameIsInComboRecipe = true;
						}
						
					}
				}
				
				if (runInitNewMoveForComboRecipe) {
					
					player.dontUpdateLastPerformedMoveNameInComboRecipe = false;
					player.lastPerformedMoveNameIsInComboRecipe = false;
					
					player.timePassedInNonFrozenFramesSinceStartOfAnim = 0;
					player.delayLastMoveWasCancelledIntoWith = 0;
					
					player.lastMoveWasJumpInstalled = jumpInstalledStage2;
					player.lastMoveWasSuperJumpInstalled = superJumpInstalledStage2;
					
					player.timeSinceWasEnableSpecialCancel = 0;
					player.timeSinceWasEnableSpecials = 0;
					
					player.moveStartTime_aswEngineTick = aswEngineTickCount;
					PlayerInfo::CancelDelay cancelDelay;
					player.determineCancelDelay(&cancelDelay);
					player.delayLastMoveWasCancelledIntoWith = cancelDelay.delay;
					player.delayInTheLastMoveIsAfterIdle = cancelDelay.isAfterIdle;
				}
				if (player.move.preservesNewSection) {
					// must clear this at the end, because we use this in PlayerInfo::determineCancelDelay
					player.timeInNewSectionForCancelDelay = 0;
				}
			}
			memcpy(player.anim, animName, 32);
			player.setMoveName(player.moveName, ent);
			// this is to update chargeable moves' names like May 6P that have many charge levels and a name reflecting each one
			if (player.moveNonEmpty
					&& player.move.displayNameSelector
					&& !player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // this to prevent Elphelt Rifle stance from turning last performed move into just "Rifle"
			) {
				player.determineMoveNameAndSlangName(&player.lastPerformedMoveName);
				moves.forCancels = true;
				player.determineMoveNameAndSlangName(&player.lastPerformedMoveNameForComboRecipe);
				moves.forCancels = false;
				if (player.lastPerformedMoveNameIsInComboRecipe && !player.comboRecipe.empty()
						&& !player.dontUpdateLastPerformedMoveNameInComboRecipe) {
					ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
					if (lastElem) {
						lastElem->name = player.lastPerformedMoveNameForComboRecipe;
					}
				}
			}
			if (player.charType == CHARACTER_TYPE_LEO
					&& strcmp(ent.gotoLabelRequests(), "SemukeEnd") == 0
					&& other.inHitstun) {
				player.comboRecipe.emplace_back();
				ComboRecipeElement& newComboElem = player.comboRecipe.back();
				newComboElem.name = assignName("Brynhildr Exit", "Backturn Exit");
				newComboElem.whiffed = false;
				newComboElem.timestamp = aswEngineTickCount;
				newComboElem.framebarId = -1;
				newComboElem.cancelDelayedBy = player.timePassedPureIdle;
				newComboElem.doneAfterIdle = true;
				player.lastPerformedMoveNameIsInComboRecipe = true;
			}
			
			if (other.pawn.inHitstun() && !startedRunOrWalkOnThisFrame
					
					// isRunning check needed to not register I-No's dash twice - it's already being registered in the run check,
					// and her dash is considered a move because she's not idle during it
					&& !player.isRunning) {
				
				// needed for Jack-O 4D1/2/3/4/6/7/8/9 - the name updates as the move goes on, without creating a new section or entering a new animation
				// a more universal mechanism will be added only if more characters need this
				if (player.charType == CHARACTER_TYPE_JACKO
						&& !player.idle && !player.isInFDWithoutBlockstun
						&& !player.comboRecipe.empty()
						&& (
							strcmp(player.anim, "IronballGenocideEx") == 0
							|| strcmp(player.anim, "AirIronballGenocideEx") == 0
						)) {
					ComboRecipeElement& lastElem = player.comboRecipe.back();
					player.determineMoveNameAndSlangName(&lastElem.name);
				}
				
				if (player.jumpCancelled
						|| player.superJumpCancelled
						|| player.jumpNonCancel
						|| player.superJumpNonCancel
						|| player.doubleJumped) {
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					
					if (player.superJumpCancelled) {
						newComboElem.name = assignName("Super Jump Cancel");
					} else if (player.jumpCancelled) {
						newComboElem.name = assignName("Jump Cancel");
					} else if (player.jumpNonCancel) {
						newComboElem.name = assignName("Jump");
					} else if (player.superJumpNonCancel) {
						newComboElem.name = assignName("Super Jump");
					} else if (player.doubleJumped) {
						newComboElem.name = assignName("Double Jump");
					}
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.artificial = true;
					newComboElem.isJump = true;
					
					// Jack-O 5H is not seen as a jump cancel, but as a 'Jump'
					if (player.timeSinceWasEnableJumpCancel) {
						newComboElem.cancelDelayedBy = player.timeSinceWasEnableJumpCancel;
					} else if (player.jumpNonCancel || player.superJumpNonCancel || player.doubleJumped) {
						newComboElem.doneAfterIdle = true;
						newComboElem.cancelDelayedBy = player.timePassedPureIdle;
					}
					
					player.jumpCancelled = false;
					player.superJumpCancelled = false;
					player.jumpNonCancel = false;
					player.superJumpNonCancel = false;
					player.doubleJumped = false;
					
				}
				
				// this should happen in the animation change registration above, but
				// we need an animFrame 3 check because things can be kara cancelled into FD, Blitz, specials, Burst, IK, supers (by completing motions)
				// so there are not many places we can go after changing animation, right?
				// what could happen inbetween that prevents us from reaching animFrame 3, besides kara cancelling and getting hit by something?
				// other than that animation starting outside of a combo, then ending, then combo starting and we hitting animFrame 3 on CmnActStand?
				// that we can filter with timePassedInNonFrozenFramesSinceStartOfAnim
				if ((
						player.animFrame == 3
						&& player.timePassedInNonFrozenFramesSinceStartOfAnim == 2
						// specials, supers and IKs cannot be kara cancelled at all
						|| player.animFrame == 1
						&& (
							player.pawn.dealtAttack()->type > ATTACK_TYPE_NORMAL
							|| player.charType == CHARACTER_TYPE_JOHNNY
							&& (
								// otherwise these disappear entirely on 1-2f taps
								strcmp(player.anim, "MistFinerBWalk") == 0
								|| strcmp(player.anim, "MistFinerFWalk") == 0
							)
						)
						&& player.timePassedInNonFrozenFramesSinceStartOfAnim == 0
					)
					&& !player.lastPerformedMoveNameIsInComboRecipe
				) {
					
					bool isSJInstall = player.lastMoveWasSuperJumpInstalled
						&& !player.lastMoveWasJumpInstalled;
					
					bool outrightBanSJInstall = charDoesNotCareAboutSuperJumpInstalls[player.charType]
						&& isSJInstall;
					
					if (
							(player.lastMoveWasJumpInstalled || player.lastMoveWasSuperJumpInstalled)
							&& (
								!(
									player.move.ignoreJumpInstalls
									|| player.move.ignoreSuperJumpInstalls
									&& isSJInstall
								)
								// dust enables all normals to be cancelled, which means that jump installs on them could potentially matter,
								// even if normally advised otherwise. For example, Zato 6H is a dead end normal, but on a dust combo it can cancel
								// into 2D which jump cancels.
								|| player.pawn.dustGatlingTimer()
								&& player.pawn.dealtAttack()->type == ATTACK_TYPE_NORMAL
							)
							&& !outrightBanSJInstall
					) {
						// Here's why I don't think I should be worried about jump installs not getting registered in the
						// Combo Recipe panel due to the normal that was jump installed getting kara cancelled into a special,
						// if that normal was a 'dead end' normal that does not care about jump installs, or a normal that does not
						// care about superjumps, while the special does care about one of those two things:
						// 1) This can happen from neutral, or when the initial normal was done raw.
						//    You must be holding UP for this when the normal starts, there's simply no other way.
						//    Or else you have to jump install on an earlier move, that we'd register.
						//    When you're holding UP, there's no way in hell you're humanly capable of completing
						//    some motion.
						//    If you do manage to cancel the prejump into some special, the panel will show the 'Jump Cancel' then.
						//    So we're not losing any information.
						// 2) This can happen from another move that you cancel. Most jump cancels extend all the way
						//    across all of the active frames, or the gatling window. Which means that you can
						//    UP+X jump install a late gatling and that is a valid jump install if the initial normal
						//    was jump cancellable. You will then face the same problem of completing any motion at all
						//    from the UP direction. Sure, if it's a charge move with 46 input, you could probably do it,
						//    like May c.S > UP+5H~Horizontal Dolphin. But I don't expect people to do this that often.
						//    The other way you could jump cancel here is during hitstop and then try to complete a motion,
						//    but then right after hitstop you'd be in prejump, and even if you cancel that prejump into a special,
						//    the 'Jump Cancel' would get registered in the Combo Recipe. So we're not losing any information.
						player.comboRecipe.emplace_back();
						ComboRecipeElement& newElem = player.comboRecipe.back();
						newElem.artificial = true;
						newElem.name = player.lastMoveWasJumpInstalled ? assignName("Jump Install") : assignName("Super Jump Install");
						newElem.timestamp = player.moveStartTime_aswEngineTick;
						if (player.lastMoveWasSuperJumpInstalled) {
							newElem.isSuperJumpInstall = true;
						}
					}
					
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					ComboRecipeElement* newComboElemPtr;
					if (!player.comboRecipe.empty()
							&& player.moveStartTime_aswEngineTick <= player.comboRecipe.back().timestamp) {
						auto newIt = player.comboRecipe.insert(player.comboRecipe.begin() + (player.comboRecipe.size() - 1), ComboRecipeElement{});
						newComboElemPtr = &*newIt;
					} else {
						player.comboRecipe.emplace_back();
						newComboElemPtr = &player.comboRecipe.back();
					}
					ComboRecipeElement& newComboElem = *newComboElemPtr;
					newComboElem.name = player.lastPerformedMoveNameForComboRecipe;
					newComboElem.whiffed = !player.hitSomething;
					newComboElem.timestamp = player.moveStartTime_aswEngineTick;
					newComboElem.framebarId = -1;
					newComboElem.cancelDelayedBy = player.delayLastMoveWasCancelledIntoWith;
					newComboElem.doneAfterIdle = player.delayInTheLastMoveIsAfterIdle;
					player.lastPerformedMoveNameIsInComboRecipe = true;
				}
				
			}
			
			if (player.moveNonEmpty && player.move.charge) {
				ChargeData charge;
				charge.elpheltShotgunChargeSkippedFrames = 0;
				player.move.charge(player, &charge);
				if (charge.current || charge.max) {
					player.charge = charge;
				}
			}
			if (player.lastPerformedMoveNameIsInComboRecipe && !player.comboRecipe.empty()
					&& (player.charge.current || player.charge.max)) {
				ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
				if (lastElem) {
					unsigned char chargeVal = minmax(0, 255, player.charge.current);
					unsigned char maxChargeVal = minmax(0, 255, player.charge.max);
					if (player.charType == CHARACTER_TYPE_ELPHELT) {
						if (strncmp(player.anim, "CounterGuard", 12) == 0) {
							
							// shield's charge
							lastElem->charge = chargeVal;
							lastElem->maxCharge = maxChargeVal;
							
							// shotgun's charge
							if (player.idle && !player.playerval1) {
								player.elpheltShotgunCharge.current = player.timePassedPureIdle + 1;
								player.elpheltShotgunCharge.max = 9  // idle recovery of Standing and Crouching Blitzes
									+ 13;  // how much you must charge in CmnActStand
								player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames = player.elpheltSkippedTimePassed;
							} else if (player.elpheltShotgunCharge.max) {
								lastElem->shotgunMaxCharge = minmax(0, 255, player.elpheltShotgunCharge.max);
								lastElem->shotgunChargeSkippedFrames = minmax(0, 255, player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames);
								player.elpheltShotgunCharge.current = 0;
								player.elpheltShotgunCharge.max = 0;
								player.elpheltShotgunCharge.elpheltShotgunChargeSkippedFrames = 0;
							}
						} else if (strncmp(player.anim, "Rifle", 5) == 0) {
							lastElem->charge = chargeVal;
							lastElem->maxCharge = maxChargeVal;
						} else {
							lastElem->shotgunMaxCharge = maxChargeVal;
							lastElem->shotgunChargeSkippedFrames = minmax(0, 255, player.charge.elpheltShotgunChargeSkippedFrames);
						}
					} else {
						lastElem->charge = chargeVal;
						lastElem->maxCharge = maxChargeVal;
					}
				}
			}
			
			if (player.cmnActIndex == CmnActJumpPre && !player.performingBDC) {
				player.prejumped = true;
			}
			
			// This is needed for animations that create projectiles on frame 1
			for (EndSceneStoredState::OccuredEvent& event : cs->events) {
				if (event.type == EndSceneStoredState::OccuredEvent::SET_ANIM
						&& needDisableProjectiles
						&& event.u.setAnim.pawn == ent) {
					for (ProjectileInfo& projectile : cs->projectiles) {
						bool parentDisabled = false;
						if (projectile.creator && !projectile.creator.isPawn()) {
							for (ProjectileInfo& seekParent : cs->projectiles) {
								if (seekParent.ptr && seekParent.ptr != projectile.ptr && seekParent.ptr == projectile.creator) {
									parentDisabled = seekParent.disabled;
									break;
								}
							}
						}
						if (projectile.team == player.index && (projectile.lifeTimeCounter > 0 || parentDisabled)) {
							projectile.disabled = true;
							projectile.startedUp = false;
							projectile.actives.clear();
							projectile.hitOnFrame = 0;
							projectile.maxHit.clear();
						}
					}
				} else if (event.type == EndSceneStoredState::OccuredEvent::SIGNAL) {
					PlayerInfo& playerFrom = findPlayer(event.u.signal.from);
					ProjectileInfo& projectileFrom = findProjectile(event.u.signal.from);
					ProjectileInfo& projectileTo = findProjectile(event.u.signal.to);
					if (playerFrom.pawn
							&& projectileTo.ptr
							&& playerFrom.pawn == ent
							&& projectileTo.team == player.index) {
						if (projectileTo.disabled) {
							projectileTo.creationTime_aswEngineTick = aswEngineTickCount;
						}
						projectileTo.disabled = false;
						projectileTo.startup = player.total + player.prevStartups.total();  // + prevStartups.total() needed for Venom QV
						if (player.totalCanBlock > player.total) {  // needed for Answer Taunt
							projectileTo.startup += player.totalCanBlock - player.total;
						}
						projectileTo.total = projectileTo.startup;
						memcpy(projectileTo.creatorName, event.u.signal.fromAnim, 32);
						projectileTo.creator = event.u.signal.from;
						projectileTo.creatorNamePtr = event.u.signal.creatorName;
					} else if (projectileFrom.ptr
							&& projectileTo.ptr
							&& projectileFrom.team == player.index
							&& projectileTo.team == player.index) {
						if (projectileTo.disabled) {
							projectileTo.creationTime_aswEngineTick = aswEngineTickCount;
						}
						projectileTo.disabled = projectileFrom.disabled;  // Sol Gunflame YRC delay 1f 5P. This prevents it from thinking Gunflame active frames are part of 5P
						projectileTo.startup = projectileFrom.total;
						projectileTo.total = projectileFrom.total;
						memcpy(projectileTo.creatorName, event.u.signal.fromAnim, 32);
						projectileTo.creator = event.u.signal.from;
						projectileTo.creatorNamePtr = event.u.signal.creatorName;
					}
				}
			}
			
			if (!player.hitstop
					&& getSuperflashInstigator() == nullptr
					&& !player.startedUp
					&& !player.inNewMoveSection
					&& !(
						player.charType == CHARACTER_TYPE_ELPHELT
						&& strcmp(player.anim, "Rifle_Roman") == 0
						&& player.idle
						&& player.canBlock
					)
					&& player.move.sectionSeparator
					&& player.move.sectionSeparator(player)) {
				player.inNewMoveSection = true;
				player.changedAnimOnThisFrame = true;  // for framebar
				player.changedAnimFiltered = true;  // for framebar
				player.timeInNewSection = 0;
				player.timeInNewSectionForCancelDelay = 0;
				if (!player.startedUp && player.total) {
					if (player.superfreezeStartup) {
						player.prevStartups.add(player.superfreezeStartup,
							player.lastMoveIsPartOfStance,
							player.lastPerformedMoveName);
						player.total -= player.superfreezeStartup;  // needed for Johnny Treasure Hunt
						player.superfreezeStartup = 0;
					}
					if (player.total) {
						player.prevStartups.add(player.total,
							player.lastMoveIsPartOfStance,
							player.lastPerformedMoveName);
					}
					player.startup = 0;
					player.total = 0;
					player.totalForInvul = 0;
					player.stoppedMeasuringInvuls = false;
					player.hitOnFrame = 0;
					player.totalCanBlock = 0;
					player.totalCanFD = 0;
				}
			}
			if (!player.idleInNewSection && player.isIdleInNewSection()) {
				player.idleInNewSection = true;
				if (!player.idlePlus) {
					player.idlePlus = true;
					player.timePassed = player.timeInNewSection;
					if (!player.frameAdvantageIncludesIdlenessInNewSection) {
						player.frameAdvantageIncludesIdlenessInNewSection = true;
						player.frameAdvantage += player.timeInNewSection;
						player.frameAdvantageNoPreBlockstun += player.timeInNewSection;
					}
					if (!other.frameAdvantageIncludesIdlenessInNewSection) {
						other.frameAdvantageIncludesIdlenessInNewSection = true;
						other.frameAdvantage -= player.timeInNewSection;
						other.frameAdvantageNoPreBlockstun -= player.timeInNewSection;
					}
					if (!other.eddie.frameAdvantageIncludesIdlenessInNewSection) {
						other.eddie.frameAdvantageIncludesIdlenessInNewSection = true;
						other.eddie.frameAdvantage -= player.timeInNewSection;
					}
					if (!player.airborne) {
						if (!player.landingFrameAdvantageIncludesIdlenessInNewSection) {
							player.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							player.landingFrameAdvantage += player.timeInNewSection;
							player.landingFrameAdvantageNoPreBlockstun += player.timeInNewSection;
						}
						if (!other.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.landingFrameAdvantage -= player.timeInNewSection;
							other.landingFrameAdvantageNoPreBlockstun -= player.timeInNewSection;
						}
						if (!other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection) {
							other.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = true;
							other.eddie.landingFrameAdvantage -= player.timeInNewSection;
						}
					}
				}
			}
			
			if (!ent.isSuperFrozen() && !ent.isRCFrozen() && !player.hitstop
					&& player.timePassedInNonFrozenFramesSinceStartOfAnim != 0xFFFFFFFF) {
				++player.timePassedInNonFrozenFramesSinceStartOfAnim;
			}
			
			if (player.wasEnableSpecialCancel
					&& player.wasEnableGatlings
					&& player.wasAttackCollidedSoCanCancelNow
					// hitting stuff resets the delay of the cancel
					&& !player.pawn.hitSomethingOnThisFrame()) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableSpecialCancel;
				}
			} else {
				player.timeSinceWasEnableSpecialCancel = 0;
			}
			
			if (player.wasEnableSpecials) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableSpecials;
				}
			} else {
				player.timeSinceWasEnableSpecials = 0;
			}
			
			if (
				(
					player.wasEnableJumpCancel
					|| player.wasCancels.hasCancel("CmnVJump")
					|| player.wasCancels.hasCancel("CmnVAirJump")
				)
				&& !player.pawn.hitSomethingOnThisFrame()
			) {
				if (!player.hitstop && !superflashInstigator) {
					++player.timeSinceWasEnableJumpCancel;
				}
			} else {
				player.timeSinceWasEnableJumpCancel = 0;
			}
			
		}
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			
			player.receivedComboCountTensionGainModifier = entityManager.calculateReceivedComboCountTensionGainModifier(
				player.inHitstunNowOrNextFrame,
				player.pawn.comboCount());
			player.dealtComboCountTensionGainModifier = entityManager.calculateDealtComboCountTensionGainModifier(
				cs->players[1 - i].inHitstunNowOrNextFrame,
				cs->players[1 - i].pawn.comboCount());
			player.burstGainOnly20Percent = player.pawn.burstGainOnly20Percent();
			player.burstGainModifier = (player.pawn.comboCount() + 32) * 100 / 32;
			if (game.isStylish(player.pawn)) {
				player.stylishBurstGainModifier = game.getStylishBurstGainModifier();
			} else {
				player.stylishBurstGainModifier = 100;
			}
		}
		
		checkVenomBallActivations();
		checkSelfProjectileHarmInflictions();
		
		// This is down here because throughout the logic tick in various hooks we gather
		// events that refer to objects via pointer, and some objects like May beachball
		// can get deleted on the same frame they receive a signal if they also happen
		// to hit something on that frame, so we process events before setting
		// deleted projectiles' ptr to null
		std::vector<ProjectileInfo>& projectileArray = cs->projectiles;
		for (auto it = projectileArray.begin(); it != projectileArray.end();) {
			bool found = false;
			ProjectileInfo& projectile = *it;
			if (projectile.ptr) {
				for (int i = 0; i < entityList.count; ++i) {
					Entity ent = entityList.list[i];
					if (ent.isActive() && projectile.ptr == ent) {
						found = true;
						break;
					}
				}
			}
			bool hadHitboxes = projectile.landedHit || objHasAttackHitboxes(projectile.ptr);
			if (!found) {
				if (hadHitboxes || projectile.gotHitOnThisFrame) {
					projectile.ptr = nullptr;
					projectile.markActive = hadHitboxes;
					if (projectile.moveNonEmpty && (projectile.team == 0 || projectile.team == 1)) {
						PlayerInfo& player = cs->players[projectile.team];
						player.registerCreatedProjectile(projectile);
					}
					++it;
					continue;
				}
				it = projectileArray.erase(it);
			} else {
				projectile.fill(projectile.ptr, superflashInstigator, false);
				projectile.markActive = hadHitboxes;
				if (projectile.team == 0 || projectile.team == 1) {
					if (!projectile.moveNonEmpty) {
						projectile.isDangerous = false;
					} else {
						projectile.isDangerous = projectile.move.isDangerous && projectile.move.isDangerous(projectile.ptr);
						PlayerInfo& player = cs->players[projectile.team];
						if (!projectile.disabled && projectile.isDangerous) {
							player.hasDangerousNonDisabledProjectiles = true;
						}
						player.registerCreatedProjectile(projectile);
					}
				}
				if (!projectile.inNewSection
						&& projectile.move.sectionSeparatorProjectile
						&& projectile.move.sectionSeparatorProjectile(projectile.ptr)) {
					projectile.inNewSection = true;
					if (projectile.total) {
						projectile.prevStartups.add(projectile.total, false, projectile.lastName);
					}
					projectile.determineMoveNameAndSlangName(&projectile.lastName);
					projectile.startup = 0;
					projectile.total = 0;
					projectile.hitOnFrame = 0;
				}
				++it;
			}
		}
		
		// This is a separate loop because it depends on another player's idlePlus, which I changed in the previous loop
		for (PlayerInfo& player : cs->players) {
			PlayerInfo& other = cs->players[1 - player.index];
			
			bool actualIdle = player.idle || player.idleInNewSection;
			
			if (player.idleLanding != actualIdle
					&& !(!player.idleLanding && player.airborne)) {  // we can't change idleLanding from false to true while player is airborne
				player.idleLanding = actualIdle;
				if (player.idleLanding && player.isLanding) {
					if (other.idleLanding) {  // no need to worry about the value of other.idleLanding being outdated here,
						                      // because if they're landing on the same frame, they will enter the if branch
						                      // immediately below and reset measuringLandingFrameAdvantage to -1
						cs->measuringLandingFrameAdvantage = -1;
						if (other.timePassedLanding > player.timePassedLanding) {
							player.landingFrameAdvantage = -player.timePassedLanding;
							player.landingFrameAdvantageNoPreBlockstun = -player.timePassedLanding;
						} else {
							player.landingFrameAdvantage = -other.timePassedLanding;
							player.landingFrameAdvantageNoPreBlockstun = -other.timePassedLanding;
						}
						other.landingFrameAdvantage = -player.landingFrameAdvantage;
						other.landingFrameAdvantageNoPreBlockstun = -player.landingFrameAdvantage;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					} else if (cs->measuringLandingFrameAdvantage == -1) {
						other.landingFrameAdvantageIncludesIdlenessInNewSection = false;
						player.landingFrameAdvantageIncludesIdlenessInNewSection = false;
						cs->measuringLandingFrameAdvantage = player.index;
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
						player.landingFrameAdvantageNoPreBlockstun = 0;
						other.landingFrameAdvantageNoPreBlockstun = 0;
						other.eddie.landingFrameAdvantageValid = false;
					}
				}
				if (player.idleInNewSection) {
					player.timePassedLanding = player.timeInNewSection;
				} else {
					player.timePassedLanding = 0;
				}
				if (player.idleLanding) {
					player.airteched = false;
					player.regainedAirOptions = false;
				}
			}
			
			if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
				
				int playerval0 = player.wasPlayerval[0];
				int playerval1 = player.wasPlayerval[1];
				int playerval2 = player.wasPlayerval[2];
				int playerval3 = player.wasPlayerval[3];
				
				const char* anim;
				const char* subAnim;
				int timeLeft;
				int slowdown;
				bool isKowareSonoba;
				int elapsed;
				bool isTrance;
				bool isCalvados;
				
				struct BitInfo {
					StringWithLength stateName;
					StringWithLength stateName2;
					StringWithLength stateName3;
					int hasSword;
					int swordDeployed;
					int stackIndex;
					std::vector<Moves::RamlethalSwordInfo>& info;
					std::vector<Moves::RamlethalSwordInfo>& info2;
					const char** anim;
					const char** subanim;
					int* time;
					int* timeMax;
					bool blockstunLinked = false;
					bool fallOnHitstun = false;
					bool recoilOnHitstun = false;
					bool isInvulnerable = false;
				};
				BitInfo bitInfos[2] {
					{
						"BitN6C",
						"BitN2C_Bunri",
						"Bit4C",
						playerval0,
						playerval1,
						0,
						moves.ramlethalBitN6C,
						moves.ramlethalBitN2C,
						&player.ramlethalSSwordAnim,
						&player.ramlethalSSwordSubanim,
						&player.ramlethalSSwordTime,
						&player.ramlethalSSwordTimeMax
					},
					{
						"BitF6D",
						"BitF2D_Bunri",
						"Bit4D",
						playerval2,
						playerval3,
						1,
						moves.ramlethalBitF6D,
						moves.ramlethalBitF2D,
						&player.ramlethalHSwordAnim,
						&player.ramlethalHSwordSubanim,
						&player.ramlethalHSwordTime,
						&player.ramlethalHSwordTimeMax
					}
				};
				
				static const char* subAnims[Moves::ram_number_of_elements] {
					"Undefined",  // ram_undefined
					"Teleporting",  // ram_teleport
					"Attack",  // ram_Attack
					"Hit Not Deployed",  // ram_koware_soubi
					"Hit Deployed",  // ram_koware_sonoba
					"Falling",  // ram_loop
					"Landing",  // ram_landing
					"Ramlethal Blocked",  // ram_koware_nokezori
					"Win"  // ram_Win
				};
				
				static const char* subAnims2[Moves::ram2_number_of_elements] {
					"Undefined",  // ram2_undefined
					"Teleporting",  // ram2_teleport
					"Attack",  // ram2_Attack
					"Win",  // ram2_Win
					"Hit",  // ram2_koware
					"Falling",  // ram2_loop
					"Landing",  // ram2_landing
					"Ramlethal Blocked"  // ram2_koware_nokezori
				};
				
				for (int j = 0; j < 2; ++j) {
					BitInfo& bitInfo = bitInfos[j];
					
					anim = "???";
					subAnim = nullptr;
					timeLeft = 0;
					slowdown = 0;
					isKowareSonoba = false;
					elapsed = 0;
					isTrance = false;
					isCalvados = false;
					
					Entity p = player.pawn.stackEntity(bitInfo.stackIndex);
					
					if (p && p.isActive()) {
						bitInfo.blockstunLinked = p.hasUpon(BBSCREVENT_PLAYER_BLOCKED);
						bitInfo.fallOnHitstun = false;
						bitInfo.recoilOnHitstun = false;
						bitInfo.isInvulnerable = p.fullInvul() || p.strikeInvul() || p.hitboxes()->count[HITBOXTYPE_HURTBOX] == 0;
						anim = p.animationName();
						if (bitInfo.stackIndex == 0) {
							isTrance = strcmp(anim, "BitSpiral_NSpiral") == 0;
						} else {
							isTrance = strcmp(anim, "BitSpiral_FSpiral") == 0;
						}
						if (!isTrance) {
							if (bitInfo.stackIndex == 0) {
								isCalvados = strcmp(anim, "BitNLaser") == 0;
							} else {
								isCalvados = strcmp(anim, "BitFLaser") == 0;
							}
						}
						ProjectileInfo& projectile = endScene.findProjectile(p);
						if (projectile.ptr && projectile.move.displayName) {
							elapsed = projectile.ramlethalSwordElapsedTime;
							anim = projectile.move.displayName ? projectile.move.displayName->name : nullptr;
						}
						#define collectPlayerGotHitInfo(koware) \
							if (p.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) { \
								BYTE* instr = moves.skipInstr(p.uponStruct(BBSCREVENT_PLAYER_GOT_HIT)->uponInstrPtr); \
								if (moves.instrType(instr) == instr_gotoLabelRequests) { \
									const char* markerToGoTo = asInstr(instr, gotoLabelRequests)->name; \
									bitInfo.fallOnHitstun = strcmp(markerToGoTo, koware) == 0; \
									bitInfo.recoilOnHitstun = !bitInfo.fallOnHitstun; \
								} \
							}
						#define resetElapsedTimeIfOnFirstFrameOfState(frames) \
							if (offset == frames.front().offset \
									&& p.justReachedSprite()) { \
								projectile.ramlethalSwordElapsedTime = 0; \
								elapsed = 0; \
							}
						if (strcmp(p.animationName(), bitInfo.stateName.txt) == 0) {
							collectPlayerGotHitInfo("koware_sonoba")
							BYTE* func = p.bbscrCurrentFunc();
							if (!func) continue;
							moves.fillInRamlethalBitN6C_F6D(func, bitInfo.info);
							int offset = p.bbscrCurrentInstr() - func;
							bool mem45 = p.mem45();
							for (const Moves::RamlethalSwordInfo& info : bitInfo.info) {
								const Moves::MayIrukasanRidingObjectInfo& vec = mem45 ? info.framesBunri : info.framesSoubi;
								if (offset >= vec.frames.front().offset && offset <= vec.frames.back().offset) {
									subAnim = subAnims[info.state];
									if (info.state == Moves::ram_teleport) {
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter())
											+ bitInfo.info[(int)Moves::ram_Attack - 1].select(mem45).totalFrames;
									} else if (info.state == Moves::ram_Attack
											|| info.state == Moves::ram_koware_soubi
											|| info.state == Moves::ram_landing
											|| info.state == Moves::ram_koware_nokezori
											|| info.state == Moves::ram_Win) {
										if ((info.state == Moves::ram_koware_soubi || info.state == Moves::ram_koware_nokezori)) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram_koware_sonoba || info.state == Moves::ram_loop) {
										if (info.state == Moves::ram_koware_sonoba) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										isKowareSonoba = true;
										timeLeft = bitInfo.info[(int)Moves::ram_landing - 1].select(mem45).totalFrames;
									}
									if (projectile.ptr) {
										slowdown = projectile.rcSlowedDownCounter;
									}
									break;
								}
							}
						} else if (strcmp(p.animationName(), bitInfo.stateName2.txt) == 0) {
							collectPlayerGotHitInfo("koware")
							BYTE* func = p.bbscrCurrentFunc();
							if (!func) continue;
							moves.fillInRamlethalBitN6C_F6D(func, bitInfo.info2);
							int offset = p.bbscrCurrentInstr() - func;
							for (const Moves::RamlethalSwordInfo& info : bitInfo.info2) {
								const Moves::MayIrukasanRidingObjectInfo& vec = info.framesBunri;
								if (offset >= vec.frames.front().offset && offset <= vec.frames.back().offset) {
									subAnim = subAnims2[info.state];
									if (info.state == Moves::ram2_teleport) {
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter())
											+ bitInfo.info2[(int)Moves::ram2_Attack - 1].framesBunri.totalFrames;
									} else if (info.state == Moves::ram2_Attack
											|| info.state == Moves::ram2_landing
											|| info.state == Moves::ram2_koware_nokezori
											|| info.state == Moves::ram2_Win) {
										if (info.state == Moves::ram2_koware_nokezori) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										timeLeft = vec.remainingTime(offset, p.spriteFrameCounter());
									} else if (info.state == Moves::ram2_koware || info.state == Moves::ram2_loop) {
										if (info.state == Moves::ram2_koware) {
											resetElapsedTimeIfOnFirstFrameOfState(vec.frames)
										}
										isKowareSonoba = true;
										timeLeft = bitInfo.info2[(int)Moves::ram2_landing - 1].framesBunri.totalFrames;
									}
									if (projectile.ptr) {
										slowdown = projectile.rcSlowedDownCounter;
									}
									break;
								}
							}
						} else if (strcmp(p.animationName(), bitInfo.stateName3.txt) == 0) {
							timeLeft = 7 - p.currentAnimDuration() + 1;
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						} else {
							bitInfo.recoilOnHitstun = p.hasUpon(BBSCREVENT_PLAYER_GOT_HIT);
						}
						#undef collectPlayerGotHitInfo
						#undef resetElapsedTimeIfOnFirstFrameOfState
					}
					
					*bitInfo.subanim = subAnim;
					*bitInfo.anim = anim;
					
					bool timerActive = timeLeft
							|| !(bitInfo.swordDeployed || bitInfo.hasSword)
							&& !isTrance
							&& !isCalvados;
					
					if (bitInfo.stackIndex == 0) {
						player.ramlethalSSwordTimerActive = timerActive;
					} else {
						player.ramlethalHSwordTimerActive = timerActive;
					}
					
					if (timerActive) {  // it takes an extra frame for the change in PLAYERVAL to affect the player
						int result;
						int resultMax;
						int unused;
						PlayerInfo::calculateSlow(
							elapsed + 1,
							timeLeft,
							slowdown,
							&result,
							&resultMax,
							&unused);
						if (bitInfo.stackIndex == 0) {
							player.ramlethalSSwordKowareSonoba = isKowareSonoba;
						} else {
							player.ramlethalHSwordKowareSonoba = isKowareSonoba;
						}
						*bitInfo.time = result + 1;
						*bitInfo.timeMax = resultMax + 1;
					}
					
				}
				
				player.ramlethalSSwordBlockstunLinked = bitInfos[0].blockstunLinked;
				player.ramlethalSSwordFallOnHitstun = bitInfos[0].fallOnHitstun;
				player.ramlethalSSwordRecoilOnHitstun = bitInfos[0].recoilOnHitstun;
				player.ramlethalSSwordInvulnerable = bitInfos[0].isInvulnerable;
				
				player.ramlethalHSwordBlockstunLinked = bitInfos[1].blockstunLinked;
				player.ramlethalHSwordFallOnHitstun = bitInfos[1].fallOnHitstun;
				player.ramlethalHSwordRecoilOnHitstun = bitInfos[1].recoilOnHitstun;
				player.ramlethalHSwordInvulnerable = bitInfos[1].isInvulnerable;
				
			} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
				
				bool grenadeFrozen = false;
				bool foundGrenadeInGeneral = false;
				bool foundGrenade = false;
				int grenadeSlowdown = 0;
				int grenadeTimeRemaining = 0;
				
				for (ProjectileInfo& projectile : cs->projectiles) {
					if (projectile.team != player.index || !projectile.ptr) continue;
					if (strcmp(projectile.animName, "GrenadeBomb") == 0) {
						foundGrenadeInGeneral = true;
						if (!projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
							foundGrenade = true;
							grenadeFrozen = projectile.ptr.isSuperFrozen();
							grenadeSlowdown = projectile.rcSlowedDownCounter;
							if (projectile.sprite.frameMax == 1) {
								grenadeTimeRemaining = 30;
							} else {
								grenadeTimeRemaining = 29 - projectile.sprite.frame;
							}
						}
					} else if (strcmp(projectile.animName, "GrenadeBomb_Ready") == 0) {
						foundGrenadeInGeneral = true;
						if (projectile.sprite.frameMax == 1) {
							grenadeTimeRemaining = 31;
							foundGrenade = true;
							grenadeFrozen = projectile.ptr && projectile.ptr.isSuperFrozen();
						} else if (projectile.sprite.frameMax == 30) {
							grenadeTimeRemaining = 30 - projectile.sprite.frame;
							foundGrenade = true;
							grenadeFrozen = projectile.ptr && projectile.ptr.isSuperFrozen();
						}
					}
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				if (!foundGrenade) {
					if (foundGrenadeInGeneral && hasForceDisableFlag) {
						player.elpheltGrenadeRemainingWithSlow = 255;
						player.elpheltGrenadeMaxWithSlow = 255;
					} else if (hasForceDisableFlag) {
						player.elpheltGrenadeRemainingWithSlow = 1;
					} else {
						player.elpheltGrenadeRemainingWithSlow = 0;
					}
					player.elpheltGrenadeElapsed = 0;
				} else {
					int result;
					int resultMax;
					int unused;
					PlayerInfo::calculateSlow(
						player.elpheltGrenadeElapsed + 1,
						grenadeTimeRemaining,
						grenadeSlowdown,
						&result,
						&resultMax,
						&unused);
					
					if (!grenadeFrozen) ++player.elpheltGrenadeElapsed;
					
					if (result || hasForceDisableFlag) ++result;
					++resultMax;
					
					player.elpheltGrenadeRemainingWithSlow = result;
					player.elpheltGrenadeMaxWithSlow = resultMax;
				}
				
				player.elpheltRifle_AimMem46 = player.getElpheltRifle_AimMem46();
				
			} else if (player.charType == CHARACTER_TYPE_JOHNNY) {
				
				{
					bool johnnyMistFinerBuffed = player.pawn.dealtAttack()->enableGuardBreak()
						|| player.pawn.dealtAttack()->guardType == GUARD_TYPE_NONE;
					player.johnnyMistFinerBuffedOnThisFrame = !player.johnnyMistFinerBuffed && johnnyMistFinerBuffed;
					player.johnnyMistFinerBuffed = johnnyMistFinerBuffed;
					
					int playerval2 = player.wasPlayerval[2];
					int currentPlayerval2 = player.pawn.playerVal(2);
					
					int slowdown = 0;
					for (ProjectileInfo& projectile : cs->projectiles) {
						if (projectile.team == player.index && strcmp(projectile.animName, "MistKuttsuku") == 0) {
							slowdown = projectile.rcSlowedDownCounter;
							if (projectile.lifeTimeCounter == 0) {
								player.johnnyMistKuttsukuElapsed = 0;
							} else if (!(projectile.ptr && projectile.ptr.isSuperFrozen())) {
								++player.johnnyMistKuttsukuElapsed;
							}
							break;
						}
					}
					
					int timeRemaining = currentPlayerval2;
					int unused;
					PlayerInfo::calculateSlow(
						player.johnnyMistKuttsukuElapsed + 1,
						timeRemaining,
						slowdown,
						&player.johnnyMistKuttsukuTimerWithSlow,
						&player.johnnyMistKuttsukuTimerMaxWithSlow,
						&unused);
					
					if (player.johnnyMistKuttsukuTimerMaxWithSlow <= 2) {
						player.johnnyMistKuttsukuTimerWithSlow = 0;
						player.johnnyMistKuttsukuTimerMaxWithSlow = 0;
					} else {
						++player.johnnyMistKuttsukuTimerMaxWithSlow;
						if (player.johnnyMistKuttsukuTimerWithSlow || !currentPlayerval2 && playerval2) {
							++player.johnnyMistKuttsukuTimerWithSlow;
						}
					}
				}
				
				{
					int maxTime = -1;
					int animFrame = -1;
					int slowdown = 0;
					bool isSuperFrozen = false;
					bool isFrozen = false;
					for (ProjectileInfo& projectile : cs->projectiles) {
						if (projectile.team == player.index && projectile.ptr && strcmp(projectile.animName, "Mist") == 0) {
							animFrame = projectile.animFrame;
							isFrozen = projectile.ptr.isRCFrozen();
							isSuperFrozen = projectile.ptr.isSuperFrozen();
							slowdown = projectile.rcSlowedDownCounter;
							BYTE* func = projectile.ptr.bbscrCurrentFunc();
							if (!func) break;
							for (loopInstr(func)) {
								if (moves.instrType(instr) == instr_ifOperation
										&& asInstr(instr, ifOperation)->op == BBSCROP_IS_GREATER_OR_EQUAL
										&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_PLAYED_IN_STATE
										&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
									maxTime = asInstr(instr, ifOperation)->right.value;
									break;
								}
							}
							break;
						}
					}
					
					if (maxTime == -1) {
						player.johnnyMistTimerWithSlow = 0;
					} else {
						
						if (animFrame == 1 && !isFrozen) {
							player.johnnyMistElapsed = 0;
						} else if (!isSuperFrozen) {
							++player.johnnyMistElapsed;
						}
						
						int timeRemaining = maxTime - animFrame;
						int unused;
						PlayerInfo::calculateSlow(
							player.johnnyMistElapsed + 1,
							timeRemaining,
							slowdown,
							&player.johnnyMistTimerWithSlow,
							&player.johnnyMistTimerMaxWithSlow,
							&unused);
						
					}
				}
				
			} else if (player.charType == CHARACTER_TYPE_JACKO) {
				
				int slowdown = 0;
				int timeRemaining = 0;
				Entity p = player.pawn.stackEntity(3);
				if (p && p.isActive()) {
					int frames = p.framesSinceRegisteringForTheIdlingSignal() - 1;
					if (moves.jackoAegisMax == 0) {
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						for (loopInstr(func)) {
							if (moves.instrType(instr) == instr_ifOperation
									&& asInstr(instr, ifOperation)->op == BBSCROP_IS_EQUAL
									&& asInstr(instr, ifOperation)->left == BBSCRVAR_FRAMES_SINCE_REGISTERING_FOR_THE_ANIMATION_FRAME_ADVANCED_SIGNAL
									&& asInstr(instr, ifOperation)->right == BBSCRTAG_VALUE) {
								moves.jackoAegisMax = asInstr(instr, ifOperation)->right.value;
								break;
							}
						}
					}
					if (p.lifeTimeCounter() == 0) player.jackoAegisElapsed = 0;
					else if (!p.isSuperFrozen()) {
						++player.jackoAegisElapsed;
					}
					ProjectileInfo& projectile = findProjectile(p);
					if (projectile.ptr) {
						slowdown = projectile.rcSlowedDownCounter;
					}
					timeRemaining = moves.jackoAegisMax - frames;
				}
				int unused;
				PlayerInfo::calculateSlow(
					player.jackoAegisElapsed + 1,
					timeRemaining,
					slowdown,
					&player.jackoAegisTimeWithSlow,
					&player.jackoAegisTimeMaxWithSlow,
					&unused);
				if (player.jackoAegisTimeMaxWithSlow <= 2) player.jackoAegisTimeMaxWithSlow = 0;
				
				player.jackoAegisActive = player.pawn.invulnForAegisField();
				player.jackoAegisReturningIn = INT_MIN;
				if (!player.pawn.inHitstunThisFrame() && !player.pawn.invulnForAegisField()) {
					for (int i = 2; i < entityList.count; ++i) {
						Entity ent = entityList.list[i];
						if (ent.isActive() && ent.team() == player.index && strcmp(ent.animationName(), "Aigisfield") == 0) {
							player.jackoAegisReturningIn = ent.mem45();
							break;
						}
					}
				}
				
				
			} else if (player.charType == CHARACTER_TYPE_HAEHYUN) {
				
				bool foundActualBall = false;
				bool foundProjectile = false;
				bool hasIndividualFlag = false;
				int projectileTimeLeft = 0;
				int slowdown = 0;
				bool createdOnThisFrame = false;
				int prevFoundAnimDur = -1;
				bool actualBallCreatedThisFrame = false;
				bool hitstop = false;
				Entity superBalls[10] { nullptr };
				int superBallsCount = 0;
				int unused;
				bool ballFrozen = false;
				bool actualBallFrozen = false;
				
				for (int j = 2; j < entityList.count; ++j) {
					ProjectileInfo* projectile = nullptr;
					bool triedFindProjectile = false;
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index) {
						bool checkSlowdown = false;
						if (strcmp(p.animationName(), "yudodan_end") == 0) {
							if (prevFoundAnimDur == -1 || prevFoundAnimDur > (int)p.currentAnimDuration()) {
								prevFoundAnimDur = p.currentAnimDuration();
							} else {
								continue;
							}
							hasIndividualFlag = (p.forceDisableFlagsIndividual() & 0x1) != 0;
							if (!hasIndividualFlag) continue;
							foundProjectile = true;
							checkSlowdown = true;
							projectileTimeLeft = 7 + 14 - p.currentAnimDuration() + 1;
							createdOnThisFrame = p.currentAnimDuration() == 1 && !p.isRCFrozen();
							ballFrozen = p.isSuperFrozen();
						} else if (strcmp(p.animationName(), "EnergyBall") == 0) {
							actualBallCreatedThisFrame = p.currentAnimDuration() == 1 && !p.isRCFrozen();
							if (actualBallCreatedThisFrame) player.haehyunBallElapsed = 0;
							foundActualBall = true;
							checkSlowdown = true;
							projectileTimeLeft = 150 - p.framesSinceRegisteringForTheIdlingSignal() + 1;
							hitstop = p.hitstop() > 0 && !p.hitSomethingOnThisFrame();
							actualBallFrozen = p.isSuperFrozen();
						} else if (strcmp(p.animationName(), "SuperEnergyBall") == 0) {
							superBalls[superBallsCount++] = p;
							projectile = &findProjectile(p);
							triedFindProjectile = true;
							if (projectile && projectile->ptr) {
								projectile->haehyunCelestialTuningBall1 = superBallsCount == 1 || superBallsCount > 2;
								projectile->haehyunCelestialTuningBall2 = superBallsCount == 2 || superBallsCount > 2;
							}
						}
						if (checkSlowdown) {
							if (!triedFindProjectile) {
								projectile = &findProjectile(p);
							}
							if (projectile && projectile->ptr) slowdown = projectile->rcSlowedDownCounter;
						}
					}
				}
				
				if (superBallsCount) {
					qsort(superBalls, superBallsCount, sizeof Entity, LifeTimeCounterCompare);
					for (int j = 0; j < superBallsCount; ++j) {
						Entity superBall = superBalls[j];
						if (superBall.lifeTimeCounter() == 0) {
							player.haehyunSuperBallRemainingElapsed[j] = 0;
						} else if (!superBall.isSuperFrozen() && !(superBall.hitstop() && !superBall.hitSomethingOnThisFrame())) {
							++player.haehyunSuperBallRemainingElapsed[j];
						}
						
						int superBallSlowdown = 0;
						ProjectileInfo& projectile = findProjectile(superBall);
						if (projectile.ptr) {
							superBallSlowdown = projectile.rcSlowedDownCounter;
						}
						
						PlayerInfo::calculateSlow(
							player.haehyunSuperBallRemainingElapsed[j] + 1,
							240 - superBall.framesSinceRegisteringForTheIdlingSignal() + 1,
							superBallSlowdown,
							player.haehyunSuperBallRemainingTimeWithSlow + j,
							player.haehyunSuperBallRemainingTimeMaxWithSlow + j,
							&unused);
					}
				}
				
				if (superBallsCount != 10) {
					player.haehyunSuperBallRemainingTimeWithSlow[superBallsCount] = 0;
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				
				if (hasForceDisableFlag) {
					if (!ballFrozen && !foundActualBall && !createdOnThisFrame) {
					 	++player.haehyunBallElapsed;
					}
				} else {
					player.haehyunBallElapsed = 0;
				}
				
				if (!foundProjectile && !foundActualBall) {
					if (hasForceDisableFlag) {
						player.haehyunBallTimeWithSlow = 1;
					} else {
						player.haehyunBallTimeWithSlow = 0;
					}
					
					player.haehyunBallRemainingElapsed = 0;
					player.haehyunBallRemainingTimeWithSlow = 0;
				} else if (foundActualBall) {
					PlayerInfo::calculateSlow(
						1,
						7 + 14,
						slowdown,
						&unused,
						&player.haehyunBallTimeMaxWithSlow,
						&unused);
					player.haehyunBallTimeWithSlow = -1;
					++player.haehyunBallTimeMaxWithSlow;
					
					if (!actualBallCreatedThisFrame && !actualBallFrozen && !hitstop) {
						++player.haehyunBallRemainingElapsed;
					}
					
					PlayerInfo::calculateSlow(
						player.haehyunBallRemainingElapsed + 1,
						projectileTimeLeft,
						slowdown,
						&player.haehyunBallRemainingTimeWithSlow,
						&player.haehyunBallRemainingTimeMaxWithSlow,
						&unused);
				} else {
					
					if (foundActualBall) {
						projectileTimeLeft = 7 + 14;
					}
					
					int unused;
					PlayerInfo::calculateSlow(
						player.haehyunBallElapsed + 1,
						projectileTimeLeft,
						slowdown,
						&player.haehyunBallTimeWithSlow,
						&player.haehyunBallTimeMaxWithSlow,
						&unused);
					++player.haehyunBallTimeWithSlow;
					++player.haehyunBallTimeMaxWithSlow;
					
					player.haehyunBallRemainingElapsed = 0;
					player.haehyunBallRemainingTimeWithSlow = 0;
				}
				
			} else if (player.charType == CHARACTER_TYPE_RAVEN) {
				
				int timeRemaining = 0;
				int slowdown = 0;
				int needleTimeRemaining = 0;
				int needleSlowdown = 0;
				bool foundNeedleButNotNull = false;
				Entity needleObj = nullptr;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index && !p.isPawn()) {
						bool checkNeedleSlowdown = false;
						if (strcmp(p.animationName(), "SlowEffect") == 0
								&& p.createArgHikitsukiVal1() == 0) {
							if (p.lifeTimeCounter() == 0) {
								player.slowTimeElapsed = 0;
							} else if (!p.isSuperFrozen()) {
								++player.slowTimeElapsed;
							}
							int timeMax;
							if (player.wasResource >= 6) {
								timeMax = 150;
							} else if (player.wasResource >= 3) {
								timeMax = 120;
							} else {
								timeMax = 90;
							}
							timeRemaining = timeMax - p.framesSinceRegisteringForTheIdlingSignal() + 1;
							
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						} else if (strcmp(p.animationName(), "SlowNeeldeObjLand") == 0
								|| strcmp(p.animationName(), "SlowNeeldeObjAir") == 0) {
							if (strcmp(p.spriteName(), "null") == 0 && p.spriteFrameCounterMax() == 14) {
								needleTimeRemaining = p.spriteFrameCounterMax() - p.spriteFrameCounter();
								checkNeedleSlowdown = true;
							} else {
								foundNeedleButNotNull = true;
								needleTimeRemaining = 14;
							}
							needleObj = p;
						} else if (strcmp(p.animationName(), "LandSettingTypeNeedleObj") == 0
								|| strcmp(p.animationName(), "AirSettingTypeNeedleObj") == 0) {
							if (strcmp(p.spriteName(), "null") == 0 && p.spriteFrameCounterMax() == 15) {
								needleTimeRemaining = p.spriteFrameCounterMax() - p.spriteFrameCounter();
								checkNeedleSlowdown = true;
							} else {
								foundNeedleButNotNull = true;
								needleTimeRemaining = 15;
							}
							needleObj = p;
						}
						if (checkNeedleSlowdown) {
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								needleSlowdown = projectile.rcSlowedDownCounter;
							}
						}
					}
				}
				
				int result;
				int resultMax;
				int unused;
				PlayerInfo::calculateSlow(
					player.slowTimeElapsed + 1,
					timeRemaining,
					slowdown,
					&result,
					&resultMax,
					&unused);
				RavenInfo& ri = player.ravenInfo;
				ri.slowTime = result;
				ri.slowTimeMax = resultMax > 1 ? resultMax : 0;
				ri.hasNeedle = hasLinkedProjectileOfType(player, "SlowNeeldeObjAir")
					|| hasLinkedProjectileOfType(player, "SlowNeeldeObjLand");
				ri.hasOrb = hasLinkedProjectileOfType(player, "AirSettingTypeNeedleObj")
					|| hasLinkedProjectileOfType(player, "LandSettingTypeNeedleObj");
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x1) != 0;
				
				if (foundNeedleButNotNull) {
					player.ravenNeedleTime = needleTimeRemaining + 1;
					player.ravenNeedleTimeMax = -1;
					if (needleObj.lifeTimeCounter() == 0) {
						player.ravenNeedleElapsed = 0;
					}
				} else if (needleObj) {
					if (!needleObj.isSuperFrozen()) {
						++player.ravenNeedleElapsed;
					}
					PlayerInfo::calculateSlow(
						player.ravenNeedleElapsed,
						needleTimeRemaining,
						needleSlowdown,
						&player.ravenNeedleTime,
						&player.ravenNeedleTimeMax,
						&unused);
					if (player.ravenNeedleTimeMax <= 1) player.ravenNeedleTimeMax = 0;
					else ++player.ravenNeedleTimeMax;
					if (hasForceDisableFlag || player.ravenNeedleTime) ++player.ravenNeedleTime;
				} else if (hasForceDisableFlag) {
					player.ravenNeedleTime = 1;
				} else {
					player.ravenNeedleTime = 0;
				}
				
			} else if (player.charType == CHARACTER_TYPE_DIZZY) {
				
				{  // spears
					bool foundThing = false;
					bool thingIsBomb = false;
					int slowdown = 0;
					int timeRemaining = 0;
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 4096) != 0;
					int fireSpearLifetime = -1;
					player.dizzySpearIsIce = false;
					player.dizzyFireSpearTimeMax = 0;
					bool frozen = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (p.isActive() && p.team() == player.index && !p.isPawn()) {
							bool checkSlowdown = false;
							const char* anim = p.animationName();
							if (strncmp(p.animationName(), "KinomiObj", 9) == 0) {
								if (anim[9] == 'N'
										&& anim[10] == 'e'
										&& anim[11] == 'c'
										&& anim[12] == 'r'
										&& anim[13] == 'o') {
									if (anim[14] == 'b' && anim[15] == 'o' && anim[16] == 'm' && anim[17] == 'b' && anim[18] == '\0') {
										BYTE* func = p.bbscrCurrentFunc();
										if (!func) continue;
										moves.fillDizzyKinomiNecrobomb(func);
										int newTime = moves.dizzyKinomiNecrobomb.remainingTime(p.bbscrCurrentInstr() - func, p.spriteFrameCounter());
										if (newTime > timeRemaining) {
											timeRemaining = newTime;
											checkSlowdown = true;
										}
										foundThing = true;
										thingIsBomb = true;
									} else if (!(foundThing && thingIsBomb)) {
										
										int index;
										if (anim[14] == '\0') index = 0;
										else index = anim[14] - '2' + 1;
										
										int* bombMarker = moves.dizzyKinomiNecroBombMarker + index;
										int* createBomb = moves.dizzyKinomiNecroCreateBomb + index;
										
										BYTE* func = p.bbscrCurrentFunc();
										if (!func) continue;
										moves.fillDizzyKinomiNecro(func, bombMarker, createBomb);
										
										foundThing = true;
										thingIsBomb = false;
										int offset = p.bbscrCurrentInstr() - func;
										if (offset > *bombMarker && offset < *createBomb && timeRemaining != -1) {
											int newTime = 2 - p.spriteFrameCounter();
											
											BYTE* func2 = p.findStateStart("KinomiObjNecrobomb");  // we need this
											moves.fillDizzyKinomiNecrobomb(func2);
											
											if (newTime > timeRemaining) {
												timeRemaining = newTime;
												checkSlowdown = true;
											}
										} else {
											timeRemaining = -1;
										}
										
										int lifetime = p.lifeTimeCounter();
										if (fireSpearLifetime == -1 || lifetime < fireSpearLifetime) {
											fireSpearLifetime = lifetime;
											player.dizzySpearX = p.x();
											player.dizzySpearY = p.y();
											player.dizzySpearSpeedX = p.speedX();
											player.dizzySpearSpeedY = p.speedY();
										}
										
									}
								} else {
									player.dizzySpearIsIce = true;
									player.dizzySpearX = p.x();
									player.dizzySpearY = p.y();
									player.dizzySpearSpeedX = p.speedX();
									player.dizzySpearSpeedY = p.speedY();
								}
							}
							if (checkSlowdown) {
								ProjectileInfo& projectile = findProjectile(p);
								if (projectile.ptr) {
									slowdown = projectile.rcSlowedDownCounter;
								}
								frozen = p.isSuperFrozen();
							}
						}
					}
					
					if (!hasForceDisableFlag) player.dizzyFireSpearElapsed = frozen ? 1 : 0;
					
					int unused;
					if (foundThing && !thingIsBomb) {
						if (timeRemaining == -1) {
							player.dizzyFireSpearTimeMax = -1;
						} else {
							timeRemaining += moves.dizzyKinomiNecrobomb.totalFrames;
							if (!frozen) ++player.dizzyFireSpearElapsed;
							PlayerInfo::calculateSlow(
								player.dizzyFireSpearElapsed,
								timeRemaining,
								slowdown,
								&player.dizzyFireSpearTime,
								&player.dizzyFireSpearTimeMax,
								&unused);
						}
					} else if (foundThing && thingIsBomb) {
						if (!frozen) ++player.dizzyFireSpearElapsed;
						PlayerInfo::calculateSlow(
							player.dizzyFireSpearElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyFireSpearTime,
							&player.dizzyFireSpearTimeMax,
							&unused);
					} else {
						player.dizzyFireSpearTime = 0;
					}
					
					if (player.dizzyFireSpearTime || hasForceDisableFlag) {
						++player.dizzyFireSpearTime;
					}
					if (player.dizzyFireSpearTimeMax != -1) {
						++player.dizzyFireSpearTimeMax;
					}
				}  // spears
				
				{  // scythes
					bool found = false;
					int remainingTime = -1;
					int slowdown = 0;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
								p.isActive() && p.team() == player.index && !p.isPawn()
								&& strcmp(p.animationName(), "AkariObj") == 0
						)) continue;
						
						found = true;
						
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						moves.fillDizzyAkari(func);
						
						int offset = p.bbscrCurrentInstr() - func;
						const Moves::MayIrukasanRidingObjectInfo* foundInfo = nullptr;
						
						// Index 0: Necro Startup
						// Index 1: Necro Loop
						// Index 2: Undine Startup (includes Undine travelling portion)
						// Index 3: Finish (can be entered into by timer from Necro or naturally from Undine. Is entered into on hit)
						int foundInfoIndex = -1;
						for (int k = 0; k < (int)moves.dizzyAkari.size(); ++k) {
							const Moves::MayIrukasanRidingObjectInfo& info = moves.dizzyAkari[k];
							if (offset >= info.frames.front().offset && offset <= info.frames.back().offset) {
								foundInfo = &info;
								foundInfoIndex = k;
								break;
							}
						}
						if (!foundInfo) continue;
						
						ProjectileInfo& projectile = findProjectile(p);
						if (!projectile.ptr) continue;
						
						int animDur = p.currentAnimDuration();
						bool isNecro = p.createArgHikitsukiVal1() == 0;
						bool isKoware = strcmp(p.gotoLabelRequests(), "koware") == 0;
						if (p.lifeTimeCounter() == 0) {
							player.dizzyScytheElapsed = 0;
						}
						
						if (!p.isSuperFrozen() || p.lifeTimeCounter() == 0) ++player.dizzyScytheElapsed;
						
						if (isNecro) {
							if (!isKoware && foundInfoIndex != 3) {
								remainingTime = 76 + moves.dizzyAkari[3].totalFrames - animDur + 1;
							} else if (isKoware) {
								remainingTime = 1 + moves.dizzyAkari[3].totalFrames;
							} else {
								remainingTime = moves.dizzyAkari[3].remainingTime(offset, p.spriteFrameCounter());
							}
						// Undine
						} else if (isKoware) {
							remainingTime = 1 + moves.dizzyAkari[3].totalFrames;
						} else if (foundInfoIndex == 3) {
							remainingTime = moves.dizzyAkari[3].remainingTime(offset, p.spriteFrameCounter());
						} else {
							remainingTime = 1  // one extra frame, because at the start it goes gotoLabelRequests: s32'Undine', which takes 1f to take effect
								+ moves.dizzyAkari[2].totalFrames + moves.dizzyAkari[3].totalFrames - animDur + 1;
						}
						
						slowdown = projectile.rcSlowedDownCounter;
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x800) != 0;
					if (!found) {
						player.dizzyScytheTime = hasForceDisableFlag ? 1 : 0;
					} else {
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyScytheElapsed,
							remainingTime,
							slowdown,
							&player.dizzyScytheTime,
							&player.dizzyScytheTimeMax,
							&unused);
						if (player.dizzyScytheTime || hasForceDisableFlag) {
							++player.dizzyScytheTime;
						}
						++player.dizzyScytheTimeMax;
					}
				}  // scythes
				
				{  // fish
					
					player.dizzyShieldFishSuperArmor = false;
					bool foundFish = false;
					bool fishEnding = false;
					int timeRemaining = -1;
					int slowdown = 0;
					bool frozen = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
							p.isActive() && p.team() == player.index && !p.isPawn()
						)) continue;
						
						Moves::MayIrukasanRidingObjectInfo* fishData = nullptr;
						bool fireFish = false;
						int* normal = nullptr;
						int* alt = nullptr;
						BYTE* func;
						const char* animName = p.animationName();
						if (strncmp(animName, "Hanashi", 7) == 0) {
							if (strcmp(animName, "HanashiObjC") == 0) {
								normal = &moves.dizzySFishNormal;
								alt = &moves.dizzySFishAlt;
								fireFish = true;
							} else if (strcmp(animName, "HanashiObjD") == 0) {
								normal = &moves.dizzyHFishNormal;
								alt = &moves.dizzyHFishAlt;
								fireFish = true;
							} else if (strcmp(animName, "HanashiObjA") == 0) {
								fishData = &moves.dizzyPFishEnd;
							} else if (strcmp(animName, "HanashiObjB") == 0) {
								fishData = &moves.dizzyKFishEnd;
							} else if (strcmp(animName, "HanashiObjE") == 0) {
								fishData = &moves.dizzyDFishEnd;
								player.dizzyShieldFishSuperArmor = p.superArmorEnabled();
							} else if (strcmp(animName, "HanashiKoware") == 0) {
								func = p.bbscrCurrentFunc();
								if (!func) continue;
								
								int totalLength = 0;
								for (loopInstr(func)) {
									if (moves.instrType(instr) == instr_sprite) {
										totalLength += asInstr(instr, sprite)->duration;
									}
								}
								
								foundFish = true;
								fishEnding = true;
								int newTime = totalLength - p.currentAnimDuration() + 1;
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
								continue;
							} else {
								continue;
							}
							
							foundFish = true;
							
							if (fireFish) {
								func = p.bbscrCurrentFunc();
								if (!func) continue;
								moves.fillDizzyLaserFish(func, normal, alt);
								
								bool isAlt = p.createArgHikitsukiVal1() != 0;
								fishEnding = true;
								int newTime = (isAlt ? *alt : *normal) - p.currentAnimDuration() + 1;
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
								continue;
							}
							
							func = p.bbscrCurrentFunc();
							if (!func) continue;
							moves.fillDizzyFish(func, *fishData);
							
							int offset = p.bbscrCurrentInstr() - func;
							if (offset >= fishData->frames.front().offset) {
								fishEnding = true;
								int newTime = fishData->remainingTime(offset, p.spriteFrameCounter());
								if (timeRemaining == -1 || newTime > timeRemaining) {
									ProjectileInfo& projectile = findProjectile(p);
									if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
									else slowdown = 0;
									timeRemaining = newTime;
									frozen = p.isSuperFrozen();
								}
							}
						}
						
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x400) != 0;
					if (!foundFish) {
						player.dizzyFishTime = hasForceDisableFlag ? 1 : 0;
						if (player.dizzyFishTimeMax == -1) player.dizzyFishTimeMax = 9999;
					} else if (fishEnding) {
						if (player.dizzyFishTime == 0) player.dizzyFishElapsed = frozen ? 1 : 0;
						if (!frozen) ++player.dizzyFishElapsed;
						
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyFishElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyFishTime,
							&player.dizzyFishTimeMax,
							&unused);
						
						if (player.dizzyFishTime || hasForceDisableFlag) ++player.dizzyFishTime;
						
						if (player.dizzyFishTimeMax <= 2) player.dizzyFishTimeMax = 0;
						else ++player.dizzyFishTimeMax;
					} else {
						player.dizzyFishTimeMax = -1;
					}
					
				}  // fish
				
				{  // bubbles
					bool foundBubble = false;
					int timeRemaining = -1;
					int slowdown = 0;
					bool frozen = false;
					bool needIncrementTimeRemaining = false;
					for (int j = 2; j < entityList.count; ++j) {
						Entity p = entityList.list[j];
						if (!(
							p.isActive() && p.team() == player.index
						)) continue;
						
						const char* animName = p.animationName();
						int* koware = nullptr;
						Moves::MayIrukasanRidingObjectInfo* bomb = nullptr;
						BYTE* func = p.bbscrCurrentFunc();
						if (!func) continue;
						if (strcmp(animName, "AwaPObj") == 0) {
							koware = &moves.dizzyAwaPKoware;
							bomb = &moves.dizzyAwaPBomb;
						} else if (strcmp(animName, "AwaKObj") == 0) {
							koware = &moves.dizzyAwaKKoware;
							bomb = &moves.dizzyAwaKBomb;
						}
						if (koware) {
							if (p.lifeTimeCounter() == 0) player.dizzyBubbleElapsed = p.isSuperFrozen() ? 1 : 0;
							moves.fillDizzyAwaKoware(func, koware);
							moves.fillDizzyAwaBomb(func, *bomb);
							foundBubble = true;
							frozen = p.isSuperFrozen();
							int offset = p.bbscrCurrentInstr() - func;
							if (strcmp(p.gotoLabelRequests(), "bomb") == 0) {
								timeRemaining = 1 + bomb->totalFrames;
							} else if (offset >= bomb->frames.front().offset) {
								timeRemaining = bomb->remainingTime(offset, p.spriteFrameCounter());
							} else {
								timeRemaining = 160 + *koware - p.currentAnimDuration() + 1;
								// adding this because apparently we need to finish playing all the sprite and then advance one more "frame" at the end
								needIncrementTimeRemaining = true;
							}
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								slowdown = projectile.rcSlowedDownCounter;
							}
						}
					}
					
					bool hasForceDisableFlag = (player.wasForceDisableFlags & 0x2000) != 0;
					if (!foundBubble) {
						player.dizzyBubbleTime = hasForceDisableFlag ? 1 : 0;
					} else {
						if (!frozen) ++player.dizzyBubbleElapsed;
						
						int unused;
						PlayerInfo::calculateSlow(
							player.dizzyBubbleElapsed != 0 && !timeRemaining
								? player.dizzyBubbleElapsed - 1
								: player.dizzyBubbleElapsed,
							timeRemaining,
							slowdown,
							&player.dizzyBubbleTime,
							&player.dizzyBubbleTimeMax,
							&unused);
						
						if (needIncrementTimeRemaining) {
							++player.dizzyBubbleTime;
							++player.dizzyBubbleTimeMax;
						}
						if (player.dizzyBubbleTime || hasForceDisableFlag) {
							++player.dizzyBubbleTime;
						}
						if (player.dizzyBubbleTimeMax <= 3) player.dizzyBubbleTimeMax = 0;
						else ++player.dizzyBubbleTimeMax;
					}
				}  // bubbles
				
			} else if (player.charType == CHARACTER_TYPE_ANSWER) {
				
				int timeRemaining = -1;
				int slowdown = 0;
				bool frozen = false;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (!(
						p.isActive() && p.team() == player.index && !p.isPawn()
							&& strcmp(p.animationName(), "Nin_Jitsu") == 0
					)) continue;
					
					frozen = p.isSuperFrozen();
					if (p.lifeTimeCounter() == 0) player.answerCantCardElapsed = frozen ? 1 : 0;
					
					timeRemaining = 43 - p.currentAnimDuration() + 1;
					ProjectileInfo& projectile = findProjectile(p);
					if (projectile.ptr) slowdown = projectile.rcSlowedDownCounter;
				}
				
				bool hasForceDisableFlag = (player.wasForceDisableFlags & 1) != 0;
				if (timeRemaining == -1) {
					player.answerCantCardTime = hasForceDisableFlag ? 1 : 0;
				} else {
					if (!frozen) ++player.answerCantCardElapsed;
					int unused;
					PlayerInfo::calculateSlow(
						player.answerCantCardElapsed,
						timeRemaining,
						slowdown,
						&player.answerCantCardTime,
						&player.answerCantCardTimeMax,
						&unused);
					if (hasForceDisableFlag || player.answerCantCardTime) ++player.answerCantCardTime;
					if (player.answerCantCardTimeMax <= 2) player.answerCantCardTimeMax = 0;
					else ++player.answerCantCardTimeMax;
				}
				
				
				bool hasRSFStart = false;
				if (strcmp(player.anim, "Royal_Straight_Flush") == 0) {
					if (player.animFrame > 4) {
						Entity prev = player.pawn.previousEntity();
						if (prev && strcmp(prev.animationName(), "RSF_Start") == 0) {
							Entity link = prev.linkObjectDestroyOnStateChange();
							if (!link) hasRSFStart = true;
						}
					}
				}
				if (player.answerPrevFrameRSFStart != hasRSFStart) {
					player.answerPrevFrameRSFStart = hasRSFStart;
					player.answerCreatedRSFStart = hasRSFStart;
				} else {
					player.answerCreatedRSFStart = false;
				}
				
			} else if (player.charType == CHARACTER_TYPE_MILLIA) {
				
				bool knifeExists = false;
				
				for (const ProjectileInfo& projectile : cs->projectiles) {
					if (projectile.ptr && projectile.team == player.index && strcmp(projectile.animName, "SilentForceKnife") == 0) {
						knifeExists = true;
						break;
					}
				}
				
				player.pickedUpSilentForceKnifeOnThisFrame = !knifeExists && player.prevFrameSilentForceKnifeExisted;
				
				if (player.pickedUpSilentForceKnifeOnThisFrame && cs->players[1 - player.index].inHitstunNowOrNextFrame) {
					ui.comboRecipeUpdatedOnThisFrame[player.index] = true;
					player.comboRecipe.emplace_back();
					ComboRecipeElement& newComboElem = player.comboRecipe.back();
					newComboElem.name = assignName("Pick Up Silent Force");
					newComboElem.timestamp = aswEngineTickCount;
					newComboElem.framebarId = -1;
					newComboElem.doneAfterIdle = true;
					newComboElem.cancelDelayedBy = player.timePassedPureIdle;
				}
				
				player.prevFrameSilentForceKnifeExisted = knifeExists;
				
			}
		}
		
		// This is a separate loop because it depends on another player's timePassedLanding, which I changed in the previous loop
		for (const PlayerInfo& player : cs->players) {
			if (player.startedDefending) {
				restartMeasuringFrameAdvantage(player.index);
				restartMeasuringLandingFrameAdvantage(player.index);
			}
		}
		
		if (!superflashInstigator) {
			for (ProjectileInfo& projectile : cs->projectiles) {
				if (!projectile.startedUp) {
					++projectile.startup;
				}
				++projectile.total;
				projectile.strikeInvul = true;
			}
		}
	}
	
	int playerSide = 2;
	if (gifMode.dontHideOpponentsBoxes || gifMode.dontHidePlayersBoxes) {
		playerSide = game.getPlayerSide();
	}
	
	hitDetector.prepareDrawHits();  // may delete items from endScene.attackHitboxes
	
	for (EndSceneStoredState::AttackHitbox& attackHitbox : cs->attackHitboxes) {
		attackHitbox.found = false;
	}
	
	// WARNING!
	// If the mod was injected in the middle of a round end or round start, player.pawn may be null here!!!
	// This will lead to a crash if you try to use it without checking for null.
	// Better use ent.playerEntity() instead.
	for (int i = 0; i < entityList.count; i++) {
		Entity ent = entityList.list[i];
		int team = ent.team();
		PlayerInfo& player = cs->players[team];
		Entity playerEntity = ent.playerEntity();
		if (!ent.isActive() || isEntityAlreadyDrawn(ent)) continue;

		const bool active = isActiveFull(ent);
		logOnce(fprintf(logfile, "drawing entity # %d. active: %d\n", i, (int)active));
		
		int* lastIgnoredHitNum = nullptr;
		if (i < 2 && (team == 0 || team == 1)) {
			lastIgnoredHitNum = &player.lastIgnoredHitNum;
		}
		int hitboxesCount = 0;
		DrawHitboxArrayCallParams hurtbox;
		
		EntityState entityState;
		
		bool useTheseValues = false;
		bool isSuperArmor;
		bool isFullInvul;
		if (i < 2 && (team == 0 || team == 1)) {
			isSuperArmor = player.wasSuperArmorEnabled;
			isFullInvul = player.wasFullInvul;
			useTheseValues = true;
		}
		
		int scaleX = INT_MAX;
		int scaleY = INT_MAX;
		if (gifMode.dontHideOpponentsBoxes && team != playerSide
				|| gifMode.dontHidePlayersBoxes && team == playerSide) {
			auto it = findHiddenEntity(ent);
			if (it != hiddenEntities.end()) {
				scaleX = it->scaleX;
				scaleY = it->scaleY;
			}
		}
		
		std::vector<DrawHitboxArrayCallParams> thisHitboxes;
		
		bool needCollectHitboxes = true;
		if (!ent.isPawn()) {
			for (EndSceneStoredState::AttackHitbox& attackHitbox : cs->attackHitboxes) {
				if (ent == attackHitbox.ent) {
					needCollectHitboxes = false;
					thisHitboxes.push_back(attackHitbox.hitbox);
					hitboxesCount = attackHitbox.count;
					attackHitbox.found = true;
					break;
				}
			}
		}
		
		std::vector<DrawBoxCallParams> thisPushbox;
		
		// need entityState
		collectHitboxes(ent,
			active,
			&hurtbox,
			needCollectHitboxes ? &thisHitboxes : nullptr,
			nullptr,
			nullptr,
			nullptr,
			&thisPushbox,
			nullptr,
			needCollectHitboxes ? &hitboxesCount : nullptr,
			lastIgnoredHitNum,
			&entityState,
			useTheseValues ? &isSuperArmor : nullptr,
			useTheseValues ? &isFullInvul : nullptr,
			scaleX,
			scaleY);
		
		logOnce(fputs("collectHitboxes(...) call successful\n", logfile));
		drawnEntities.push_back(ent);
		logOnce(fputs("drawnEntities.push_back(...) call successful\n", logfile));

		// Attached entities like dusts
		Entity attached = ent.effectLinkedCollision();
		if (attached != nullptr) {
			logOnce(fprintf(logfile, "Attached entity: %p\n", attached.ent));
			collectHitboxes(attached,
				active,
				nullptr,
				&thisHitboxes,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				nullptr,
				&hitboxesCount,
				lastIgnoredHitNum,
				nullptr,
				nullptr,
				nullptr,
				scaleX,
				scaleY);
			drawnEntities.push_back(attached);
		}
		if (team == 0 || team == 1) {
			if (!hitboxesCount && i < 2) {
				if (player.hitSomething) ++hitboxesCount;
			}
			RECT hitboxesBoundsTotal;
			if (hitboxesCount && !thisHitboxes.empty()) {
				bool isFirstHitbox = true;
				for (auto it = thisHitboxes.begin(); it != thisHitboxes.end(); ++it) {
					if (isFirstHitbox) {
						hitboxesBoundsTotal = it->getWorldBounds();
					} else {
						combineBounds(hitboxesBoundsTotal, it->getWorldBounds());
					}
					isFirstHitbox = false;
				}
			}
			if (i < 2) {
				player.hitboxesCount += hitboxesCount;
				if (
						player.hitboxesCount
						&& !player.comboRecipe.empty()
						&& player.lastPerformedMoveNameIsInComboRecipe
				) {
					ComboRecipeElement* lastElem = player.findLastNonProjectileComboElement();
					if (lastElem) {
						lastElem->isMeleeAttack = true;
					}
				}
				if (hitboxesCount && !thisHitboxes.empty()) {
					player.hitboxTopY = hitboxesBoundsTotal.top;
					player.hitboxBottomY = hitboxesBoundsTotal.bottom;
					player.hitboxTopBottomValid = true;
				}
				RECT hurtboxBounds;
				if (!hurtbox.data.empty()) {
					hurtboxBounds = hurtbox.getWorldBounds();
					player.hurtboxTopY = hurtboxBounds.top;
					player.hurtboxBottomY = hurtboxBounds.bottom;
					player.hurtboxTopBottomValid = true;
				}
				player.strikeInvul.active = entityState.strikeInvuln;
				player.throwInvul.active = entityState.throwInvuln;
				if (!entityState.strikeInvuln
						&& !player.airborne
						&& !hurtbox.data.empty()) {
					if (hurtboxBounds.bottom <= 88000) {
						player.superLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 159000) {
						player.lowProfile.active = true;
					} else if (hurtboxBounds.bottom < 175000) {
						player.somewhatLowProfile.active = true;
					} else if (hurtboxBounds.bottom <= 232000
							&& playerEntity.blockstun() == 0
							&& !player.inHitstunNowOrNextFrame) {
						player.upperBodyInvul.active = true;
					} else if (hurtboxBounds.bottom <= 274000
							&& playerEntity.blockstun() == 0
							&& !player.inHitstunNowOrNextFrame) {
						player.aboveWaistInvul.active = true;
					}
					if (hurtboxBounds.top >= 174000) {
						player.legInvul.active = true;
					} else if (hurtboxBounds.top >= 84000) {
						player.footInvul.active = true;
					} else if (hurtboxBounds.top >= 20000) {
						player.toeInvul.active = true;
					}
				}
				if (player.y > 0) {
					if (!hurtbox.data.empty() && hurtboxBounds.top < 20000 && !entityState.strikeInvuln) {
						player.airborneButWontGoOverLows.active = true;
					} else {
						player.airborneInvul.active = true;
					}
				}
				if (playerEntity.damageToAir() && !player.airborne && !entityState.strikeInvuln) {
					player.consideredAirborne.active = true;
				}
				player.frontLegInvul.active = !entityState.strikeInvuln
					&& player.move.frontLegInvul
					&& player.move.frontLegInvul(player);
				player.superArmor.active = entityState.superArmorActive && !player.projectileOnlyInvul.active && !player.reflect.active;
				if (player.charType == CHARACTER_TYPE_LEO
						&& player.superArmor.active
						&& strcmp(player.anim, "Semuke5E") == 0) {  // Leo bt.D
					for (int projectileSearch = 2; projectileSearch < entityList.count; ++projectileSearch) {
						Entity projectileSearchPtr = entityList.list[projectileSearch];
						if (projectileSearchPtr.isActive()
								&& projectileSearchPtr.team() == player.index
								&& strcmp(projectileSearchPtr.animationName(), "Semuke5E_Reflect") == 0
								&& projectileSearchPtr.superArmorEnabled()) {
							player.reflect.active = true;
						}
					}
				}
				if (player.superArmor.active) {
					player.superArmorThrow.active = ent.superArmorThrow();
					player.superArmorBurst.active = ent.superArmorBurst();
					player.superArmorMid.active = ent.superArmorMid();
					player.superArmorOverhead.active = ent.superArmorOverhead();
					player.superArmorLow.active = ent.superArmorLow();
					player.superArmorGuardImpossible.active = ent.superArmorGuardImpossible();
					player.superArmorObjectAttacck.active = ent.superArmorObjectAttacck();
					player.superArmorHontaiAttacck.active = ent.superArmorHontaiAttacck();
					player.superArmorProjectileLevel0.active = ent.superArmorProjectileLevel0();
					player.superArmorOverdrive.active = ent.superArmorOverdrive();
					player.superArmorBlitzBreak.active = ent.superArmorBlitzBreak();
				}
				player.counterhit = entityState.counterhit;
				player.crouching = ent.crouching();
				
				if (!thisPushbox.empty()) {
					DrawBoxCallParams& box = thisPushbox.front();
					int w;
					w = box.right - box.left;
					player.pushboxWidth = (w < 0 ? -w : w);
					w = box.bottom - box.top;
					player.pushboxHeight = (w < 0 ? -w : w);
				} else {
					player.pushboxWidth = 0;
					player.pushboxHeight = 0;
				}
				
			} else if (frameHasChanged) {
				ProjectileInfo& projectile = findProjectile(ent);
				if (projectile.ptr) {
					projectile.strikeInvul = entityState.strikeInvuln;
					if (hitboxesCount) {
						projectile.markActive = true;
						if (!thisHitboxes.empty()) {
							projectile.hitboxTopY = hitboxesBoundsTotal.top;
							projectile.hitboxBottomY = hitboxesBoundsTotal.bottom;
							projectile.hitboxTopBottomValid = true;
						}
					}
					projectile.superArmorActive = entityState.superArmorActive;
				}
			}
		}
	}
	drawnEntities.clear();
	
	if (getSuperflashInstigator() == nullptr && frameHasChanged) {
		std::vector<EndSceneStoredState::LeoParry>& leoParries = cs->leoParries;
		for (auto it = leoParries.begin(); it != leoParries.end(); ) {
			EndSceneStoredState::LeoParry& parry = *it;
			++parry.timer;
			if (parry.timer >= 12) {
				it = leoParries.erase(it);
			} else {
				++it;
			}
		}
	}
	
	logOnce(fputs("got past the entity loop\n", logfile));
	hitDetector.drawHitsInside();
	logOnce(fputs("hitDetector.drawDetected() call successful\n", logfile));
	throws.drawThrowsInside();
	logOnce(fputs("throws.drawThrows() call successful\n", logfile));
	
	if (frameHasChanged) {
		
		bool matchTimerZero = game.getMatchTimer() == 0;
		bool atLeastOneDiedThisFrame = false;
		bool atLeastOneDead = false;
		bool atLeastOneNotInHitstop = false;
		bool atLeastOneBusy = false;
		bool atLeastOneDangerousProjectilePresent = false;
		bool atLeastOneDoingGrab = false;
		bool atLeastOneGettingHitBySuper = false;
		bool atLeastOneStartedGettingHitBySuperOnThisFrame = false;
		bool hasDangerousProjectiles[2] { false };
		
		for (ProjectileInfo& projectile : cs->projectiles) {
			if (projectile.team != 0 && projectile.team != 1) {
				continue;
			}
			
			bool needUseHitstop = false;
			bool isDangerous = false;
			
			if (projectile.markActive
					|| projectile.gotHitOnThisFrame
					|| projectile.move.isDangerous
					&& projectile.move.isDangerous(projectile.ptr)) {
				isDangerous = true;
				needUseHitstop = true;
			}
			
			if (isDangerous) {
				hasDangerousProjectiles[projectile.team] = true;
				atLeastOneDangerousProjectilePresent = true;
			}
			
			if (needUseHitstop && !projectile.hitstop) {
				atLeastOneNotInHitstop = true;
			}
		}
		
		bool playerCantRc[2] { false };
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			if (player.pawn.performingThrow()) {
				playerCantRc[i] = true;
			} else if (player.pawn.romanCancelAvailability() == ROMAN_CANCEL_DISALLOWED) {
				playerCantRc[i] = true;
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			atLeastOneGettingHitBySuper = atLeastOneGettingHitBySuper
				|| playerCantRc[1 - i]
				&& player.gettingHitBySuper
				&& !hasDangerousProjectiles[i];
			atLeastOneStartedGettingHitBySuperOnThisFrame = atLeastOneStartedGettingHitBySuperOnThisFrame
				|| player.gettingHitBySuper
				&& !player.prevGettingHitBySuper;
			atLeastOneDiedThisFrame = atLeastOneDiedThisFrame || player.hp == 0 && player.prevHp != 0;
			atLeastOneDead = atLeastOneDead || player.hp == 0;
			if (
					(
						player.grab
						|| player.move.isGrab && !(player.move.forceLandingRecovery && player.landingRecovery)
					)
					&& !(
						!player.wasCancels.gatlings.empty()
						|| !player.wasCancels.whiffCancels.empty()
					)
					&& playerCantRc[i]
			) {
				if (player.pawn.dealtAttack()->type == ATTACK_TYPE_OVERDRIVE) {
					atLeastOneGettingHitBySuper = true;
				} else {
					atLeastOneDoingGrab = true;
				}
			}
			if (!player.hitstop
					|| player.charType == CHARACTER_TYPE_BAIKEN
					&& (
						!settings.ignoreHitstopForBlockingBaiken && player.blockstun > 0
						|| player.move.ignoresHitstop
					)) {
				atLeastOneNotInHitstop = true;
			}
			if (
					(
						(
							!(
								!settings.considerKnockdownWakeupAndAirtechIdle
								&& (
									player.idleLanding
									&& !(
										player.charType == CHARACTER_TYPE_JAM
										&& strcmp(player.anim, "NeoHochihu") == 0
									)
									|| player.idle
									&& player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // Chipp Wall Climb
								)
								|| settings.considerKnockdownWakeupAndAirtechIdle
								&& (player.airteched ? player.idlePlus : player.idleLanding)
							)
							|| (!player.canBlock || !player.canFaultlessDefense)
							&& !player.ignoreNextInabilityToBlockOrAttack
							&& !player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
						) && !(
							!settings.considerRunAndWalkNonIdle
							&& player.charType == CHARACTER_TYPE_LEO
							&& player.cmnActIndex == CmnActFDash
						)
						|| (
							(
								player.strikeInvul.active
								|| player.throwInvul.active
							) && (
								!settings.considerKnockdownWakeupAndAirtechIdle
								|| !player.wokeUp
								&& (player.airteched ? player.idlePlus : player.idleLanding)
							)
							|| player.projectileOnlyInvul.active
							|| player.superArmor.active
							|| player.reflect.active
						)
						&& !settings.considerIdleInvulIdle
						|| settings.considerRunAndWalkNonIdle
						&& (
							player.cmnActIndex == CmnActFWalk
							|| player.cmnActIndex == CmnActBWalk
							|| player.cmnActIndex == CmnActFDash
							|| player.cmnActIndex == CmnActFDashStop
							|| player.charType == CHARACTER_TYPE_LEO
							&& strcmp(player.anim, "Semuke") == 0
							&& (
								strcmp(player.pawn.gotoLabelRequests(), "SemukeFrontWalk") == 0
								|| strcmp(player.pawn.gotoLabelRequests(), "SemukeBackWalk") == 0
								|| player.pawn.speedX() != 0
							)
							|| (
								player.charType == CHARACTER_TYPE_FAUST
								|| player.charType == CHARACTER_TYPE_BEDMAN
							) && (
								strcmp(player.anim, "CrouchFWalk") == 0
								|| strcmp(player.anim, "CrouchBWalk") == 0
							)
							|| player.charType == CHARACTER_TYPE_HAEHYUN
							&& strcmp(player.anim, "CrouchFDash") == 0
						)
						|| settings.considerCrouchNonIdle
						&& misterPlayerIsManuallyCrouching(player)
						|| player.forceBusy
						|| settings.considerDummyPlaybackNonIdle
						&& game.isTrainingMode() && game.getDummyRecordingMode() == DUMMY_MODE_PLAYING_BACK
						|| player.charType == CHARACTER_TYPE_ELPHELT
						&& (
							player.playerval0
							&& (
								!player.playerval1
								|| !player.prevFramePlayerval1
							)
							&& (
								player.cmnActIndex == CmnActStand
								|| player.cmnActIndex == CmnActCrouch2Stand
								|| player.cmnActIndex == NotACmnAct
								&& strncmp(player.anim, "CounterGuard", 12) == 0
								&& player.y == 0
							)
							|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // rifle stance
							&& (
								!player.elpheltRifle_AimMem46
								|| player.elpheltRifle_AimMem46
								&& !player.prevFrameElpheltRifle_AimMem46
							)
						)
						|| player.wasCantBackdashTimer != 0
						|| player.cmnActIndex == CmnActJumpLanding
						&& !player.pawn.enableWalkForward()
					)
					&& (
						!settings.considerKnockdownWakeupAndAirtechIdle
						|| !(
							(
								player.cmnActIndex == CmnActBDownBound
								|| player.cmnActIndex == CmnActFDownBound
								|| player.cmnActIndex == CmnActVDownBound
							)
							&& player.pawn.isOtg()
							|| (
								player.cmnActIndex == CmnActBDownLoop
								|| player.cmnActIndex == CmnActFDownLoop
								|| player.cmnActIndex == CmnActVDownLoop
							)
							|| player.wakeupTiming  // CmnActFDown2Stand, CmnActBDown2Stand, CmnActWallHaritsukiGetUp, CmnActUkemi
						)
					)) {
				atLeastOneBusy = true;
			}
		}
		
		SkippedFramesType skippedType = SKIPPED_FRAMES_HITSTOP;
		SkippedFramesType skippedGrabType = SKIPPED_FRAMES_GRAB;
		if (atLeastOneGettingHitBySuper && !atLeastOneStartedGettingHitBySuperOnThisFrame) {
			atLeastOneDoingGrab = true;
			skippedGrabType = SKIPPED_FRAMES_SUPER;
		}
		if (atLeastOneDoingGrab && settings.skipGrabsInFramebar) {
			skippedType = skippedGrabType;
			atLeastOneBusy = false;
			atLeastOneNotInHitstop = false;
			atLeastOneDangerousProjectilePresent = false;
		}
		
		if ((atLeastOneDead || matchTimerZero) && !atLeastOneDiedThisFrame) {
			// do nothing
		} else if (getSuperflashInstigator() == nullptr) {
			int displayedFrames = settings.framebarDisplayedFramesCount;
			if (displayedFrames < 1) {
				displayedFrames = 1;
			}
			int storedFrames = settings.framebarStoredFramesCount;
			if (storedFrames < 1) {
				storedFrames = 1;
			}
			if (storedFrames > _countof(PlayerFramebar::frames)) {
				storedFrames = _countof(PlayerFramebar::frames);
			}
			if (displayedFrames > storedFrames) {
				displayedFrames = storedFrames;
			}
			
			if (atLeastOneBusy || atLeastOneDangerousProjectilePresent) {
				if (cs->framebarIdleHitstopFor > IDLE_MAX) {
					
					cs->nextSkippedFramesHitstop.clear();
					cs->nextSkippedFramesIdleHitstop.clear();
					
					EntityFramebar::incrementPos(cs->framebarPositionHitstop);
					cs->framebarTotalFramesHitstopUnlimited = 1;
					// you're not allowed to clear the framebar. We only have one for all rollback states,
					// and we must be able to rollback to any previous point we like
					for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
						EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
						entityFramebar.getHitstop().clear();
						entityFramebar.getIdleHitstop().clear();
					}
				} else {
					if (cs->framebarIdleHitstopFor) {
						
						int framebarPositionHitstopPlusOne = EntityFramebar::posPlusOne(cs->framebarPositionHitstop);
						int ind = framebarPositionHitstopPlusOne;
						for (int i = 1; i <= cs->framebarIdleHitstopFor; ++i) {
							skippedFramesHitstop[ind] = skippedFramesIdleHitstop[ind];
							EntityFramebar::incrementPos(ind);
						}
						cs->nextSkippedFramesHitstop = cs->nextSkippedFramesIdleHitstop;
						
						for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
							EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
							entityFramebar.getHitstop().catchUpToIdle(
								entityFramebar.getIdleHitstop(), framebarPositionHitstopPlusOne, cs->framebarIdleHitstopFor,
								cs->framebarTotalFramesHitstopUnlimited);
						}
						cs->framebarPositionHitstop += cs->framebarIdleHitstopFor;
						cs->framebarPositionHitstop %= (int)_countof(PlayerFramebar::frames);
					}
					EntityFramebar::incrementPos(cs->framebarPositionHitstop);
					increaseFramesCountUnlimited(cs->framebarTotalFramesHitstopUnlimited,
						1 + cs->framebarIdleHitstopFor,
						displayedFrames);
					
					skippedFramesHitstop[cs->framebarPositionHitstop] = cs->nextSkippedFramesHitstop;
					cs->nextSkippedFramesHitstop.clear();
					skippedFramesIdleHitstop[cs->framebarPositionHitstop] = cs->nextSkippedFramesIdleHitstop;
					cs->nextSkippedFramesIdleHitstop.clear();
					
					const int preFrameBuffer = _countof(PlayerFramebar::frames) - FRAMES_MAX;
					int preFramePos = EntityFramebar::confinePos(cs->framebarPositionHitstop + preFrameBuffer);
					
					bool needSoak = cs->framebarTotalFramesHitstopUnlimited > FRAMES_MAX;
					for (PlayerFramebars& framebar : playerFramebars) {
						if (needSoak) {
							framebar.hitstop.soakUpIntoPreFrame(framebar.hitstop.frames[preFramePos]);
							framebar.idleHitstop.soakUpIntoPreFrame(framebar.idleHitstop.frames[preFramePos]);
						}
						framebar.hitstop.clearCancels(cs->framebarPositionHitstop);
						framebar.idleHitstop.clearCancels(cs->framebarPositionHitstop);
					}
					// we're not allowed to advance projectile framebars here
					
				}
				cs->framebarIdleHitstopFor = 0;
				framebarAdvancedHitstop = true;
				framebarAdvancedIdleHitstop = true;
				
				if (atLeastOneNotInHitstop) {
					if (cs->framebarIdleFor > IDLE_MAX) {
						
						cs->nextSkippedFrames.clear();
						cs->nextSkippedFramesIdle.clear();
						
						EntityFramebar::incrementPos(cs->framebarPosition);
						cs->framebarTotalFramesUnlimited = 1;
						// you're not allowed to clear the framebar. Mark a point and it will be treated as empty before that point
						for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
							EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
							entityFramebar.getMain().clear();
							entityFramebar.getIdle().clear();
						}
					} else {
						if (cs->framebarIdleFor) {
							
							int framebarPositionPlusOne = EntityFramebar::posPlusOne(cs->framebarPosition);
							int ind = framebarPositionPlusOne;
							for (int i = 1; i <= cs->framebarIdleFor; ++i) {
								skippedFrames[ind] = skippedFramesIdle[ind];
								EntityFramebar::incrementPos(ind);
							}
							cs->nextSkippedFrames = cs->nextSkippedFramesIdle;
							
							for (int totalFramebarIndex = 0; totalFramebarIndex < totalFramebarCount(); ++totalFramebarIndex) {
								EntityFramebar& entityFramebar = getFramebar(totalFramebarIndex);
								bool isPlayer = entityFramebar.belongsToPlayer();
								entityFramebar.getMain().catchUpToIdle(entityFramebar.getIdle(), framebarPositionPlusOne, cs->framebarIdleFor,
									cs->framebarTotalFramesUnlimited);
							}
							cs->framebarPosition += cs->framebarIdleFor;
							cs->framebarPosition %= (int)_countof(PlayerFramebar::frames);
						}
						EntityFramebar::incrementPos(cs->framebarPosition);
						increaseFramesCountUnlimited(cs->framebarTotalFramesUnlimited,
							1 + cs->framebarIdleFor,
							displayedFrames);
						
						skippedFrames[cs->framebarPosition] = cs->nextSkippedFrames;
						cs->nextSkippedFrames.clear();
						skippedFramesIdle[cs->framebarPosition] = cs->nextSkippedFramesIdle;
						cs->nextSkippedFramesIdle.clear();
						
						const int preFrameBuffer = _countof(PlayerFramebar::frames) - FRAMES_MAX;
						int preFramePos = EntityFramebar::confinePos(cs->framebarPosition + preFrameBuffer);
						
						bool needSoak = cs->framebarTotalFramesUnlimited > FRAMES_MAX;
						for (PlayerFramebars& framebar : playerFramebars) {
							if (needSoak) {
								framebar.main.soakUpIntoPreFrame(framebar.main.frames[preFramePos]);
								framebar.idle.soakUpIntoPreFrame(framebar.idle.frames[preFramePos]);
							}
							framebar.main.clearCancels(cs->framebarPosition);
							framebar.idle.clearCancels(cs->framebarPosition);
						}
						// not allowed to advance projectile framebars unless some useful info is going to be in the frame
					}
					cs->framebarIdleFor = 0;
					framebarAdvanced = true;
					framebarAdvancedIdle = true;
					if (!settings.neverIgnoreHitstop) ui.onFramebarAdvanced();
				} else {  // if not  atLeastOneNotInHitstop
					cs->nextSkippedFrames.addFrame(SKIPPED_FRAMES_HITSTOP);
					cs->nextSkippedFramesIdle.addFrame(SKIPPED_FRAMES_HITSTOP);
				}
				if (settings.neverIgnoreHitstop) ui.onFramebarAdvanced();
			} else if (atLeastOneDoingGrab) {  // if not (atLeastOneBusy || atLeastOneDangerousProjectilePresent)
				cs->nextSkippedFrames.addFrame(skippedType);
				cs->nextSkippedFramesIdle.addFrame(skippedType);
				cs->nextSkippedFramesHitstop.addFrame(skippedType);
				cs->nextSkippedFramesIdleHitstop.addFrame(skippedType);
			} else {
				framebarAdvancedIdleHitstop = true;
				++cs->framebarIdleHitstopFor;
				int confinedPos = EntityFramebar::confinePos(cs->framebarPositionHitstop + cs->framebarIdleHitstopFor);
				skippedFramesIdleHitstop[confinedPos] = cs->nextSkippedFramesIdleHitstop;
				cs->nextSkippedFramesIdleHitstop.clear();
				const int preFrameBuffer = _countof(PlayerFramebar::frames) - FRAMES_MAX;
				int preFramePos = EntityFramebar::confinePos(confinedPos + preFrameBuffer);
				bool needSoak = cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor > FRAMES_MAX;
				for (PlayerFramebars& framebar : playerFramebars) {
					if (needSoak) {
						framebar.idleHitstop.soakUpIntoPreFrame(framebar.idleHitstop.frames[preFramePos]);
					}
					framebar.idleHitstop.clearCancels(confinedPos);
				}
				// not allowed to advance projectile framebars
				if (atLeastOneNotInHitstop) {
					framebarAdvancedIdle = true;
					++cs->framebarIdleFor;
					confinedPos = EntityFramebar::confinePos(cs->framebarPosition + cs->framebarIdleFor);
					skippedFramesIdle[confinedPos] = cs->nextSkippedFramesIdle;
					cs->nextSkippedFramesIdle.clear();
					const int preFrameBuffer = _countof(PlayerFramebar::frames) - FRAMES_MAX;
					int preFramePos = EntityFramebar::confinePos(confinedPos + preFrameBuffer);
					bool needSoak = cs->framebarTotalFramesUnlimited + cs->framebarIdleFor > FRAMES_MAX;
					for (PlayerFramebars& framebar : playerFramebars) {
						if (needSoak) {
							framebar.idle.soakUpIntoPreFrame(framebar.idle.frames[preFramePos]);
						}
						framebar.idle.clearCancels(confinedPos);
					}
					// not allowed to advance projectile framebars
				} else {  // if not atLeastOneNotInHitstop
					cs->nextSkippedFrames.addFrame(skippedType);
					cs->nextSkippedFramesIdle.addFrame(skippedType);
				}
			}
		} else {  // if not getSuperflashInstigator() == nullptr
			cs->nextSkippedFrames.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			cs->nextSkippedFramesIdle.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			cs->nextSkippedFramesHitstop.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
			cs->nextSkippedFramesIdleHitstop.addFrame(SKIPPED_FRAMES_SUPERFREEZE);
		}
			
		Entity superflashInstigator = getSuperflashInstigator();
		
		if (!superflashInstigator) {
			for (PlayerInfo& player : cs->players) {
				if (!player.hitstop) {  // needed for Axl DP
					player.activesProj.beginMergeFrame();
				}
			}
		}
		
		int instigatorTeam = -1;
		if (superflashInstigator) {
			instigatorTeam = superflashInstigator.team();
			
			int newValue;
			
			newValue = getSuperflashCounterAllied();
			if (newValue > cs->superflashCounterAllied) {
				cs->superflashCounterAlliedMax = newValue;
			}
			cs->superflashCounterAllied = newValue;
			
			newValue = getSuperflashCounterOpponent();
			if (newValue > cs->superflashCounterOpponent) {
				cs->superflashCounterOpponentMax = newValue;
			}
			cs->superflashCounterOpponent = newValue;
			
		} else {
			cs->superflashCounterAllied = 0;
			cs->superflashCounterOpponent = 0;
		}
		
		for (ThreadUnsafeSharedPtr<ProjectileFramebar>& framebar : projectileFramebars) {
			framebar->foundOnThisFrame = false;
		}
		const int framebarPos = (cs->framebarPositionHitstop + cs->framebarIdleHitstopFor) % _countof(PlayerFramebar::frames);
		int framebarHitstopSearchPos = cs->framebarPositionHitstop;
		if (framebarAdvancedHitstop) EntityFramebar::decrementPos(framebarHitstopSearchPos);
		int framebarPosNoHitstop = (cs->framebarPosition + cs->framebarIdleFor) % _countof(PlayerFramebar::frames);
		int framebarIdleSearchPos = framebarPosNoHitstop;
		if (framebarAdvancedIdle) EntityFramebar::decrementPos(framebarIdleSearchPos);
		int framebarSearchPos = cs->framebarPosition;
		if (framebarAdvanced) EntityFramebar::decrementPos(framebarSearchPos);
		
		for (ProjectileInfo& projectile : cs->projectiles) {
			bool ignoreThisForPlayer = false;
			PlayerInfo& player = projectile.team == 0 || projectile.team == 1 ? cs->players[projectile.team] : cs->players[0];
			if (projectile.team == 0 || projectile.team == 1) {
				ignoreThisForPlayer = 
					player.charType == CHARACTER_TYPE_JACKO
					&& (
						strcmp(projectile.animName, "ServantA") == 0
						|| strcmp(projectile.animName, "ServantB") == 0
						|| strcmp(projectile.animName, "ServantC") == 0
						|| strcmp(projectile.animName, "magicAtkLv1") == 0
						|| strcmp(projectile.animName, "magicAtkLv2") == 0
						|| strcmp(projectile.animName, "magicAtkLv3") == 0
					);
			}
			
			if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1)
					&& !projectile.prevStartups.empty()
					&& !ignoreThisForPlayer) {
				
				player.prevStartupsProj = projectile.prevStartups;
				for (int i = 0; i < player.prevStartupsProj.count; ++i) {
					player.prevStartupsProj[i].moveName = nullptr;
				}
			}
			
			bool projectileCanBeHit = false;
			if ((
					projectile.isRamlethalSword
					|| player.charType == CHARACTER_TYPE_ELPHELT
					&& strcmp(projectile.animName, "GrenadeBomb") == 0
					&& projectile.ptr
					&& projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)
					|| player.charType == CHARACTER_TYPE_JACKO
					&& projectile.ptr
					&& projectile.ptr.servant()
					|| player.charType == CHARACTER_TYPE_DIZZY
					&& strncmp(projectile.animName, "HanashiObj", 10) == 0
					// unfortunately, the code that finds Eddie happens later, so we have to re-find Eddie here
					|| player.charType == CHARACTER_TYPE_ZATO
					&& (
						strcmp(projectile.animName, "ChouDoriru") == 0
						|| player.pawn.playerVal(0)
						&& projectile.ptr
						&& projectile.ptr == getReferredEntity((void*)player.pawn.ent, ENT_STACK_0)
					)
				) && !projectile.strikeInvul
				|| projectile.gotHitOnThisFrame
				&& !isDizzyBubble(projectile.animName)
				&& !isVenomBall(projectile.animName)) {
				projectileCanBeHit = true;
			}
			bool isHouseInvul = player.charType == CHARACTER_TYPE_JACKO && projectile.ptr && projectile.ptr.ghost()
				&& projectile.strikeInvul && projectile.ptr.displayModel()
				&& projectile.ptr.mem45() != 0  // first-time create
				&& projectile.ptr.mem45() != 3;  // second-time create of a previously retrieved house
			bool isMist;
			bool isMistKuttsuku;
			if (player.charType == CHARACTER_TYPE_JOHNNY) {
				if (strcmp(projectile.animName, "Mist") == 0) {
					isMist = true;
					isMistKuttsuku = false;
				} else if (strcmp(projectile.animName, "MistKuttsuku") == 0) {
					isMist = false;
					isMistKuttsuku = projectile.animFrame == 1 && (!projectile.ptr || !projectile.ptr.isRCFrozen());
				} else {
					isMist = false;
					isMistKuttsuku = false;
				}
			} else {
				isMist = false;
				isMistKuttsuku = false;
			}
			
			ProjectileFramebar& entityFramebar = findProjectileFramebar(projectile,
				projectile.markActive
				|| projectile.gotHitOnThisFrame
				|| projectileCanBeHit
				|| isMist
				|| isMistKuttsuku
				|| isHouseInvul);
			entityFramebar.foundOnThisFrame = true;
			Framebar& framebar = entityFramebar.idleHitstop;
			
			FramebarTitle prevTitle;
			Frame* currentFramePtr;
			// this exists because of belief that projectile framebars have become so complex you're only allowed to call advance once per actual frame
			if (&entityFramebar != &defaultFramebar) {
				int posToFeedToThisDumbFunction = framebarAdvancedIdleHitstop
					? EntityFramebar::posMinusOne(framebarPos)
					: framebarPos;
				framebar.getLastTitle(posToFeedToThisDumbFunction, &prevTitle);
				
				posToFeedToThisDumbFunction = framebarAdvancedIdleHitstop
					? framebarPos
					: EntityFramebar::posPlusOne(framebarPos);
				int totalFrameCountToFeedToThisIdiot = framebarAdvancedIdleHitstop
					? cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor - 1
					: cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor;
				if (framebar.stateHead->idleTime) {
					framebar.convertIdleTimeToFrames(posToFeedToThisDumbFunction, totalFrameCountToFeedToThisIdiot, &prevTitle);
				}
				if (framebarAdvancedIdleHitstop) {
					framebar.advance(framebarPos, cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor);
				}
				currentFramePtr = &framebar.makeSureFrameExists(framebarPos);
			} else {
				static Frame defaultFrameStorage;
				currentFramePtr = &defaultFrameStorage;
			}
			Frame& currentFrame = *currentFramePtr;
			
			FrameType defaultIdleFrame;
			if (isHouseInvul && !projectileCanBeHit
					|| projectile.gotHitOnThisFrame && (
						isDizzyBubble(projectile.animName) || isVenomBall(projectile.animName) || isGrenadeBomb(projectile.animName)
					)) {
				defaultIdleFrame = FT_IDLE_NO_DISPOSE;
			} else if (isMist || isMistKuttsuku) {
				defaultIdleFrame = FT_BACCHUS_SIGH;
			} else {
				defaultIdleFrame = projectileCanBeHit ? FT_IDLE_PROJECTILE_HITTABLE : FT_IDLE_PROJECTILE;
			}
			
			if (framebarAdvancedIdleHitstop
					|| (
						projectile.markActive
						&& !projectile.startedUp
						&& superflashInstigator
					)) {
				currentFrame.type = defaultIdleFrame;
				currentFrame.marker = isHouseInvul
						|| player.charType == CHARACTER_TYPE_DIZZY
						&& projectileCanBeHit
						&& player.dizzyShieldFishSuperArmor;
				if (projectile.ptr || !projectile.lastName) {
					projectile.determineMoveNameAndSlangName(&currentFrame.animName);
				} else {
					currentFrame.animName = projectile.lastName;
				}
				currentFrame.hitstop = projectile.hitstopWithSlow;
				currentFrame.hitstopMax = projectile.hitstopMaxWithSlow;
				currentFrame.hitConnected = projectile.hitConnectedForFramebar() || projectile.gotHitOnThisFrame
					|| isMistKuttsuku;
				currentFrame.rcSlowdown = projectile.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = projectile.rcSlowedDownMax;
				currentFrame.activeDuringSuperfreeze = false;
				currentFrame.powerup = projectile.move.projectilePowerup && projectile.move.projectilePowerup(projectile);
				currentFrame.next = nullptr;
				currentFrame.charSpecific1 = player.charType == CHARACTER_TYPE_RAMLETHAL
					&& projectile.ptr
					&& projectile.ptr == player.pawn.stackEntity(0)
					|| player.charType == CHARACTER_TYPE_HAEHYUN
					&& projectile.haehyunCelestialTuningBall1
					|| player.charType == CHARACTER_TYPE_ELPHELT
					&& (
						currentFrame.animName == PROJECTILE_NAME_BERRY
						|| currentFrame.animName == PROJECTILE_NAME_BERRY_READY
					);
				currentFrame.charSpecific2 = player.charType == CHARACTER_TYPE_RAMLETHAL
					&& projectile.ptr
					&& projectile.ptr == player.pawn.stackEntity(1)
					|| player.charType == CHARACTER_TYPE_HAEHYUN
					&& projectile.haehyunCelestialTuningBall2;
				if (!projectile.dontReplaceFramebarTitle
						|| currentFrame.title.text == nullptr || *currentFrame.title.text->name == '\0') {
					currentFrame.title = projectile.framebarTitle;
				} else if (framebarPos != 0 || cs->framebarTotalFramesHitstopUnlimited > 0 || cs->framebarIdleHitstopFor > 0) {
					currentFrame.title = prevTitle;
				}
			}
			if (!framebarAdvancedIdleHitstop && superflashInstigator && projectile.gotHitOnThisFrame) {
				currentFrame.hitConnected = true;
				if (currentFrame.type == FT_NONE) {
					currentFrame.type = defaultIdleFrame;
				}
			}
			
			if (!projectile.markActive) {
				if (!superflashInstigator) {
					projectile.actives.addNonActive();
					if (framebarAdvancedIdleHitstop) {
						currentFrame.type = defaultIdleFrame;
					}
				}
			} else if (!superflashInstigator || !projectile.startedUp) {
				if (!projectile.startedUp) {
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
						if (!player.startupProj) {
							player.startupProj = projectile.startup;
						}
					}
					projectile.startedUp = true;
				}
				if (projectile.actives.count) {
					int lastNonActives = projectile.actives.data[projectile.actives.count - 1].nonActives;
					if (lastNonActives && &entityFramebar != &defaultFramebar) {
						entityFramebar.changePreviousFramesOneType(defaultIdleFrame,
							FT_NON_ACTIVE_PROJECTILE,
							framebarPos - 1,
							framebarHitstopSearchPos,
							framebarIdleSearchPos,
							framebarSearchPos,
							lastNonActives,
							framebarPos,
							cs->framebarPositionHitstop,
							framebarPosNoHitstop,
							cs->framebarPosition,
							cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
							cs->framebarTotalFramesHitstopUnlimited,
							cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
							cs->framebarTotalFramesUnlimited);
					}
				}
				if (projectile.actives.count
						&& !projectile.actives.data[projectile.actives.count - 1].nonActives
						&& projectile.hitNumber != projectile.actives.prevHitNum
						&& !(projectile.ptr ? projectile.ptr.isRCFrozen() : false)) {
					framebar.stateHead->requestNextHit = true;
				}
				projectile.actives.addActive(projectile.hitNumber);
				if (!projectile.hitOnFrame && projectile.hitConnectedForFramebar()) {
					projectile.hitOnFrame = projectile.actives.total();
					if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
						if (!player.hitOnFrameProj) {
							player.hitOnFrameProj = projectile.hitOnFrame;
						}
					}
				}
				if (framebarAdvancedIdleHitstop) {
					currentFrame.type = projectile.hitstop ? FT_ACTIVE_HITSTOP_PROJECTILE : FT_ACTIVE_PROJECTILE;
				}
				if (superflashInstigator && !framebarAdvancedIdleHitstop) {
					currentFrame.activeDuringSuperfreeze = true;
					if (currentFrame.type == defaultIdleFrame || currentFrame.type == FT_NONE) {
						currentFrame.type = FT_IDLE_NO_DISPOSE;
					}
				}
				if (!projectile.disabled && (projectile.team == 0 || projectile.team == 1) && !ignoreThisForPlayer) {
					if (!player.hitstop) {
						if (superflashInstigator) {
							player.activesProj.addSuperfreezeActive(projectile.hitNumber);
						} else {
							player.activesProj.addActive(projectile.hitNumber);
						}
						if (player.maxHitProj.empty()) {
							if (!player.maxHitProjConflict) {
								player.maxHitProj = projectile.maxHit;
								player.maxHitProjLastPtr = projectile.ptr;
							}
						} else if (!projectile.maxHit.empty()) {
							if (projectile.ptr == player.maxHitProjLastPtr) {
								if (projectile.ptr) {
									player.maxHitProj = projectile.maxHit;
								} else if (player.maxHitProj != projectile.maxHit) {
									player.maxHitProjConflict = true;
									player.maxHitProj.clear();
								}
							} else if (!projectile.ptr ? player.maxHitProj != projectile.maxHit : true) {
								player.maxHitProjConflict = true;
								player.maxHitProj.clear();
							}
						}
					}
				}
			}
			
			if (framebarAdvancedIdleHitstop) {
				currentFrame.newHit = framebar.stateHead->requestNextHit;
			}
			
			if (&entityFramebar != &defaultFramebar) {
				copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
					framebarAdvanced,
					framebarAdvancedIdle,
					framebarAdvancedHitstop,
					framebarAdvancedIdleHitstop);
			}
			
		}
			
		if (framebarAdvancedIdleHitstop) {
			for (ThreadUnsafeSharedPtr<ProjectileFramebar>& entityFramebar : projectileFramebars) {
				if (entityFramebar->foundOnThisFrame) continue;
				
				++entityFramebar->idleHitstop.stateHead->idleTime;
				if (framebarAdvancedIdle) {
					++entityFramebar->idle.stateHead->idleTime;
				}
				if (framebarAdvancedHitstop) {
					++entityFramebar->hitstop.stateHead->idleTime;
				}
				if (framebarAdvanced) {
					++entityFramebar->main.stateHead->idleTime;
				}
			}
		}
		
		if (!superflashInstigator) {
			for (PlayerInfo& player : cs->players) {
				if (!player.hitstop) {
					player.activesProj.endMergeFrame();
				}
				
				if (player.charType != CHARACTER_TYPE_ZATO) continue;
				
				int resource = player.pawn.exGaugeValue(0);
				int prevResource = player.eddie.prevResource;
				player.eddie.prevResource = resource;
				PlayerInfo& enemy = cs->players[1 - player.index];
				Entity ent = nullptr;
				if (player.pawn.playerVal(0)) {
					ent = getReferredEntity((void*)player.pawn.ent, ENT_STACK_0);
				}
				bool created = false;
				
				Entity landminePtr = nullptr;
				for (ProjectileInfo& projectile : cs->projectiles) {
					if (strcmp(projectile.animName, "ChouDoriru") == 0
							&& projectile.ptr
							&& projectile.team == player.index) {
						landminePtr = projectile.ptr;
						if (!player.eddie.landminePtr) {
							projectile.creationTime_aswEngineTick = player.eddie.moveStartTime_aswEngineTick;
							projectile.startup = player.eddie.total;
							memset(projectile.creatorName, 0, 32);
							strcpy(projectile.creatorName, "Eddie");
							projectile.creator = player.eddie.ptr;
							projectile.creatorNamePtr = "Eddie";
							projectile.total = player.eddie.total;
						}
						break;
					}
				}
				player.eddie.landminePtr = landminePtr;
				
				if (player.eddie.ptr != ent) {
					player.eddie.ptr = ent;
					created = !player.eddie.landminePtr;
				}
				if (!player.eddie.ptr && !landminePtr) continue;
				
				int diff = prevResource - resource;
				if (diff > 10) {  // 10 is the amount you lose each frame
					player.eddie.consumedResource = diff;
				}
				
				bool idleNext = !player.eddie.ptr ? false : !player.pawn.playerVal(2);
				ProjectileInfo& projectile = !player.eddie.ptr ? findProjectile(landminePtr) : findProjectile(player.eddie.ptr);
				if (created || strcmp(player.eddie.anim, projectile.animName) != 0 || !idleNext && projectile.animFrame < player.eddie.prevAnimFrame) {
					memcpy(player.eddie.anim, projectile.animName, 32);
					
					if (created || !idleNext && player.eddie.ptr) {
						projectile.alreadyIncludedInComboRecipe = false;
						player.eddie.total = 0;
						player.eddie.hitOnFrame = 0;
						player.eddie.moveStartTime_aswEngineTick = aswEngineTickCount;
						player.eddie.startup = 0;
						player.eddie.startedUp = false;
						player.eddie.actives.clear();
						player.eddie.maxHit.clear();
						player.eddie.recovery = 0;
						player.eddie.frameAdvantageValid = false;
						player.eddie.landingFrameAdvantageValid = false;
						player.eddie.frameAdvantageIncludesIdlenessInNewSection = false;
						player.eddie.landingFrameAdvantageIncludesIdlenessInNewSection = false;
					}
					
					if (created) {
						player.eddie.hitstopMax = 0;
						player.eddie.moveStartTime_aswEngineTick = player.moveStartTime_aswEngineTick;
						player.eddie.startup = player.total;
						player.eddie.total = player.total;
						player.eddie.prevEnemyIdle = enemy.idlePlus;
						player.eddie.prevEnemyIdleLanding = enemy.idleLanding;
					}
				}
				player.eddie.prevAnimFrame = projectile.animFrame;
				
				if (created || player.eddie.idle != idleNext) {
					player.eddie.idle = idleNext;
					player.eddie.timePassed = 0;
					if (idleNext && enemy.idlePlus) {
						player.eddie.frameAdvantage = player.eddie.timePassed - enemy.timePassed;
						if (enemy.timePassed < 999) {
							player.eddie.frameAdvantageValid = true;
						}
					}
					if (idleNext && enemy.idleLanding) {
						player.eddie.landingFrameAdvantage = player.eddie.timePassed - enemy.timePassedLanding;
						if (enemy.timePassedLanding < 999) {
							player.eddie.landingFrameAdvantageValid = true;
						}
					}
				}
				if (enemy.idlePlus != player.eddie.prevEnemyIdle) {
					player.eddie.prevEnemyIdle = enemy.idlePlus;
					if (enemy.idlePlus && player.eddie.idle) {
						player.eddie.frameAdvantage = player.eddie.timePassed - enemy.timePassed;
						player.eddie.frameAdvantageValid = true;
					}
				}
				if (enemy.idleLanding != player.eddie.prevEnemyIdleLanding) {
					player.eddie.prevEnemyIdleLanding = enemy.idleLanding;
					if (enemy.idleLanding && player.eddie.idle) {
						player.eddie.landingFrameAdvantage = player.eddie.timePassed - enemy.timePassedLanding;
						player.eddie.landingFrameAdvantageValid = true;
					}
				}
				
				int hitstop = player.eddie.hitstop;
				player.eddie.hitstop = projectile.hitstop;
				if (!hitstop && projectile.hitstop) {
					player.eddie.hitstopMax = projectile.hitstop;
				}
				
				if (!projectile.hitstop && !player.eddie.idle) {
					if (!player.eddie.startedUp) {
						++player.eddie.startup;
					}
					if (!projectile.markActive) {
						if (player.eddie.startedUp) {
							++player.eddie.recovery;
						}
					} else {
						player.eddie.startedUp = true;
						if (player.eddie.recovery) {
							player.eddie.actives.addNonActive(player.eddie.recovery);
							player.eddie.recovery = 0;
						}
						if (!player.eddie.hitOnFrame && projectile.hitConnectedForFramebar()) {
							player.eddie.hitOnFrame = player.eddie.total - player.eddie.startup + 1;
						}
						player.eddie.maxHit = projectile.maxHit;
						player.eddie.actives.addActive(projectile.hitNumber, 1);
					}
					++player.eddie.total;
				}
				
				++player.eddie.timePassed;
				
				if (framebarAdvancedIdleHitstop) {
					
					ProjectileFramebar& entityFramebar = findProjectileFramebar(projectile, true);
					Framebar& framebar = entityFramebar.idleHitstop;
					if (!entityFramebar.foundOnThisFrame) {
						if (framebar.stateHead->idleTime) {
							framebar.convertIdleTimeToFrames(framebarPos, cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor - 1);
						}
						framebar.advance(framebarPos, cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor);
					}
					Frame& currentFrame = framebar.makeSureFrameExists(framebarPos);
					FrameType oldType = currentFrame.type;
					if (idleNext) {
						currentFrame.type = FT_EDDIE_IDLE;
					} else if (!player.eddie.actives.count) {
						currentFrame.type = FT_EDDIE_STARTUP;
					} else if (!player.eddie.recovery) {
						currentFrame.type = player.eddie.hitstop ? FT_EDDIE_ACTIVE_HITSTOP : FT_EDDIE_ACTIVE;
					} else {
						currentFrame.type = FT_EDDIE_RECOVERY;
					}
					if (oldType != currentFrame.type) {
						if (!entityFramebar.foundOnThisFrame) {
							// the framebar was created by us, the Eddie-specific code
							copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
								framebarAdvanced,
								framebarAdvancedIdle,
								framebarAdvancedHitstop,
								framebarAdvancedIdleHitstop);
						} else {
							// the framebar was created by projectile-handling code, and all framebars are already caught up.
							// copyIdleHitstopFrameToTheRestOfSubframebars function should not be called more than once per frame
							entityFramebar.hitstop.modifyFrame(cs->framebarPositionHitstop, currentFrame.aswEngineTick, currentFrame.type);
							entityFramebar.idle.modifyFrame(
								EntityFramebar::confinePos(cs->framebarPosition + cs->framebarIdleFor),
								currentFrame.aswEngineTick, currentFrame.type);
							entityFramebar.main.modifyFrame(cs->framebarPosition, currentFrame.aswEngineTick, currentFrame.type);
						}
					}
				}
				
			}
		}
		
		for (int i = 0; i < 2; ++i) {
			PlayerInfo& player = cs->players[i];
			PlayerInfo& other = cs->players[1 - i];
			
			if (!superflashInstigator) {
				if (player.idle && !player.isRunning && !player.isWalkingForward && !player.isWalkingBackward) {
					if (!player.hitstop) {
						if (other.inHitstun) {
							++player.timePassedPureIdle;
						} else {
							++player.elpheltSkippedTimePassed;
						}
					}
				} else {
					player.timePassedPureIdle = 0;
					player.elpheltSkippedTimePassed = 0;
				}
				++player.timePassed;
				++player.timePassedLanding;
				if (player.inNewMoveSection) {
					if (!player.isWalkingForward && !player.isWalkingBackward) {
						++player.timeInNewSectionForCancelDelay;
					}
					++player.timeInNewSection;
				}
				if (player.timeSinceLastGap == 0) {
					if (other.idle || !other.onTheDefensive) {
						player.timeSinceLastGap = 1;
					}
				} else {
					++player.timeSinceLastGap;
				}
				
				if (player.idlePlus && !other.idlePlus) {
					if (cs->measuringFrameAdvantage) {
						++player.frameAdvantage;
						++player.frameAdvantageNoPreBlockstun;
					}
				} else if (!player.idlePlus && other.idlePlus) {
					if (cs->measuringFrameAdvantage) {
						--player.frameAdvantage;
						--player.frameAdvantageNoPreBlockstun;
					}
				} else if (!player.idlePlus && !other.idlePlus) {
					if (cs->measuringLandingFrameAdvantage == -1) {
						player.landingFrameAdvantageValid = false;
						other.landingFrameAdvantageValid = false;
					}
					player.frameAdvantage = 0;
					other.frameAdvantage = 0;
					player.frameAdvantageNoPreBlockstun = 0;
					other.frameAdvantageNoPreBlockstun = 0;
					cs->measuringFrameAdvantage = true;
					player.frameAdvantageValid = false;
					other.frameAdvantageValid = false;
					player.frameAdvantageIncludesIdlenessInNewSection = false;
					other.frameAdvantageIncludesIdlenessInNewSection = false;
				} else if (player.idlePlus && other.idlePlus) {
					if (cs->measuringFrameAdvantage) {
						cs->measuringFrameAdvantage = false;
						player.frameAdvantageValid = true;
						other.frameAdvantageValid = true;
					}
				}
				
				if (i == cs->measuringLandingFrameAdvantage) {
					if (player.idleLanding && !other.idleLanding) {
						++player.landingFrameAdvantage;
						--other.landingFrameAdvantage;
						++player.landingFrameAdvantageNoPreBlockstun;
						--other.landingFrameAdvantageNoPreBlockstun;
					} else if (!player.idleLanding && other.idleLanding) {
						--player.landingFrameAdvantage;
						++other.landingFrameAdvantage;
						--player.landingFrameAdvantageNoPreBlockstun;
						++other.landingFrameAdvantageNoPreBlockstun;
					} else if (!player.idleLanding && !other.idleLanding) {
						player.landingFrameAdvantage = 0;
						other.landingFrameAdvantage = 0;
						player.landingFrameAdvantageNoPreBlockstun = 0;
						other.landingFrameAdvantageNoPreBlockstun = 0;
					} else if (player.idleLanding && other.idleLanding) {
						cs->measuringLandingFrameAdvantage = -1;
						player.landingFrameAdvantageValid = true;
						other.landingFrameAdvantageValid = true;
					}
				}
				
			}
			
			if (player.charType == CHARACTER_TYPE_INO) {
				player.noteTime = -1;
				int noteElapsedTime = 0;
				int noteSlowdown = 0;
				for (int j = 2; j < entityList.count; ++j) {
					Entity p = entityList.list[j];
					if (p.isActive() && p.team() == player.index && !p.isPawn() && strcmp(p.animationName(), "Onpu") == 0) {
						if (!(
									p.mem45() && strcmp(p.gotoLabelRequests(), "hit") != 0
						)) {
							player.noteTime = p.currentAnimDuration();
							ProjectileInfo& projectile = findProjectile(p);
							if (projectile.ptr) {
								noteElapsedTime = projectile.elapsedTime + 1;
								noteSlowdown = projectile.rcSlowedDownCounter;
							}
						}
						break;
					}
				}
				if (player.noteTime != -1) {
					player.lastNoteTime = player.noteTime;
					if (player.noteTime >= 68) {
						player.noteLevel = 5;
						player.noteTimeMax = -1;
					} else if (player.noteTime >= 56) {
						player.noteLevel = 4;
						player.noteTimeMax = 68;
					} else if (player.noteTime >= 44) {
						player.noteLevel = 3;
						player.noteTimeMax = 56;
					} else if (player.noteTime >= 32) {
						player.noteLevel = 2;
						player.noteTimeMax = 44;
					} else {
						player.noteLevel = 1;
						player.noteTimeMax = 32;
					}
					
					if (player.noteTimeMax != -1) {
						int result;
						int unused;
						PlayerInfo::calculateSlow(
							noteElapsedTime,
							player.noteTimeMax - player.noteTime,
							noteSlowdown,
							&result,
							&unused,
							&unused);
						player.noteTimeWithSlow = noteElapsedTime;
						player.noteTimeWithSlowMax = noteElapsedTime + result;
					} else {
						player.noteTimeWithSlow = noteElapsedTime;
						player.noteTimeWithSlowMax = -1;
					}
				}
			}
			
			PlayerFramebars& entityFramebar = playerFramebars[player.index];
			PlayerFramebar& framebar = entityFramebar.idleHitstop;
			
			bool inXStun = player.inHitstun
				|| player.blockstun > 0
				|| player.wakeupTiming;
			if (player.changedAnimOnThisFrame
					&& (
						player.startup == 0
						|| player.changedAnimFiltered
						|| player.cmnActIndex == CmnActJumpPre
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActJump
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActJumpLanding
						&& player.animFrame == 1
						|| player.cmnActIndex == CmnActLandingStiff
						&& player.animFrame == 1
					)
					&& !player.pawn.isRCFrozen()) {
				framebar.stateHead->requestFirstFrame = true;
			}
			
			PlayerFrame& prevFrame = framebar[EntityFramebar::posMinusOne(framebarPos)];
			if (player.wasEnableNormals
					&& !player.canBlock) {
				if (prevFrame.type != FT_NONE && !prevFrame.canBlock && !prevFrame.enableNormals) {
					framebar.stateHead->requestFirstFrame = true;
				}
			}
			
			FrameType hasCancelsRecoveryFrameType = FT_NONE;
			bool hasCancelsOverridePrevRecovery = false;
			bool hasCancels = false;
			if (player.move.isRecoveryCanReload && player.move.isRecoveryCanReload(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_CAN_RELOAD;
				hasCancels = true;
			} else if (player.move.isRecoveryCanAct && player.move.isRecoveryCanAct(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_CAN_ACT;
				hasCancels = true;
			} else if (player.move.isRecoveryHasGatlings
					&& player.move.isRecoveryHasGatlings(player)) {
				hasCancelsRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
				hasCancels = true;
				
			// Automatic version of isRecoveryHasGatlings
			} else if (!(
						player.move.isRecoveryHasGatlings
						|| player.move.isRecoveryCanAct
						|| player.move.isRecoveryCanReload
					)
					&& (
						player.wasEnableGatlings
						&& player.wasAttackCollidedSoCanCancelNow
						&& (
							!player.wasCancels.gatlings.empty()
							|| player.wasEnableSpecialCancel
							|| player.wasClashCancelTimer
						)
						|| player.wasEnableWhiffCancels
						&& !player.wasCancels.whiffCancels.empty()
					)) {
				if (player.recovery >= 1) {
					hasCancelsRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
					hasCancelsOverridePrevRecovery = true;
				}
				hasCancels = true;
			}
			
			PlayerFrame& currentFrame = framebar[framebarPos];
			FrameType defaultStartupFrame = FT_STARTUP;
			FrameType defaultActiveFrame = player.hitstop ? FT_ACTIVE_HITSTOP : FT_ACTIVE;
			FrameType defaultRecoveryFrameType = FT_RECOVERY;
			bool overrideStartupFrame = false;
			bool isAzami = player.move.ignoresHitstop;
			FrameType standardXStun;
			if (player.wasEnableAirtech && player.hitstun == 0) {
				standardXStun = FT_GRAYBEAT_AIR_HITSTUN;
			} else if (player.hitstop) {
				standardXStun = FT_XSTUN_HITSTOP;
			} else if (player.charType == CHARACTER_TYPE_BAIKEN
					&& player.blockstun
					&& !player.wasCancels.whiffCancels.empty()) {
				standardXStun = FT_XSTUN_CAN_CANCEL;
			} else {
				standardXStun = FT_XSTUN;
			}
			
			FrameType startupFrameType;
			if (player.canBlock) {
				if (hasCancels) {
					if (blitzShieldCancellable(player, false)
							&& !player.hitstop
							&& isBlitzPostHitstopFrame_outsideTick(player)) {
						startupFrameType = FT_STARTUP_CAN_BLOCK;
					} else {
						startupFrameType = FT_STARTUP_CAN_BLOCK_AND_CANCEL;
					}
				} else {
					startupFrameType = FT_STARTUP_CAN_BLOCK;
				}
			} else {
				startupFrameType = FT_STARTUP;
			}
			
			Entity ent = entityList.slots[i];
			bool hasHitboxes = player.hitboxesCount > 0
				|| player.charType == CHARACTER_TYPE_ANSWER
				&& strcmp(player.anim, "Zaneiken") == 0
				&& player.hitstop
				&& hitDetector.hasHitboxThatHit(player.pawn);
			bool enableSpecialCancel = player.wasEnableSpecialCancel
					&& player.wasAttackCollidedSoCanCancelNow;
					//&& player.wasEnableGatlings;  // Jack-O 5H can special cancel even without this flag
					// I also checked the game's code and it does not use the gatlings flag for special cancels
			bool hitAlreadyHappened = player.pawn.hitAlreadyHappened() >= player.pawn.theValueHitAlreadyHappenedIsComparedAgainst()
					|| !player.pawn.currentHitNum();
			player.getInputs(game.getInputRingBuffers() + player.index, isTheFirstFrameInTheRound);
			
			if (framebarAdvancedIdleHitstop) {
				
				FrameType zatoFrameType = FT_NONE;
				if (player.move.zatoHoldLevel && !player.idle) {
					DWORD zatoHoldLevel = player.move.zatoHoldLevel(player);
					if ((zatoHoldLevel & 3) >= 2) {
						if ((zatoHoldLevel & 4) != 0) {
							zatoFrameType = (FrameType)((char)FT_ZATO_BREAK_THE_LAW_STAGE2_RELEASED + (zatoHoldLevel & 3) - 2);
						} else {
							zatoFrameType = (FrameType)((char)FT_ZATO_BREAK_THE_LAW_STAGE2 + (zatoHoldLevel & 3) - 2);
						}
					}
				}
				
				currentFrame.canYrc = player.wasCanYrc;
				currentFrame.cantRc = player.wasCantRc;
				// for Ramlethal BitLaser, we redefine this in character-specific codes below
				player.canYrcProjectile = player.wasCanYrc && player.move.canYrcProjectile
					? player.move.canYrcProjectile(player) : nullptr;
				currentFrame.canYrcProjectile = player.canYrcProjectile;
				
				MilliaInfo milliaInfo { 0 };
				if (player.charType == CHARACTER_TYPE_MILLIA) {
					player.milliaChromingRoseTimeLeft = player.wasPlayerval1Idling;
					if (player.wasPlayerval1Idling) {
						player.milliaChromingRoseTimeLeft += 10;
					} else {
						for (int j = 2; j < entityList.count; ++j) {
							Entity p = entityList.list[j];
							if (p.isActive() && p.team() == i && !p.isPawn()
									&& strcmp(p.animationName(), "RoseBody") == 0
									&& strcmp(p.spriteName(), "null") == 0) {
								if (p.spriteFrameCounterMax() == 10) {
									player.milliaChromingRoseTimeLeft += 11 - p.spriteFrameCounter();
								} else {
									++player.milliaChromingRoseTimeLeft;
								}
							}
						}
					}
					milliaInfo = player.canProgramSecretGarden();
					milliaInfo.chromingRose = player.milliaChromingRoseTimeLeft;
					milliaInfo.chromingRoseMax = player.maxDI;
					milliaInfo.hasPin = hasProjectileOfTypeAndHasNotExhausedHit(player, "SilentForceKnife");
					milliaInfo.hasSDisc = hasProjectileOfType(player, "TandemTopCRing");
					milliaInfo.hasHDisc = hasProjectileOfType(player, "TandemTopDRing");
					milliaInfo.hasEmeraldRain = hasAnyProjectileOfTypeStrNCmp(player, "EmeraldRainRing");
					milliaInfo.hasHitstunLinkedSecretGarden = hasLinkedProjectileOfType(player, "SecretGardenBall");
					milliaInfo.hasRose = hasAnyProjectileOfType(player, "RoseObj");
					currentFrame.u.milliaInfo = milliaInfo;
				} else if (player.charType == CHARACTER_TYPE_CHIPP) {
					ChippInfo& ci = currentFrame.u.chippInfo;
					ci.invis = player.playerval0;
					ci.wallTime = 0;
					if (player.move.caresAboutWall) {
						int wallTime = player.pawn.mem54();
						if (wallTime > USHRT_MAX || wallTime <= 0) wallTime = USHRT_MAX;
						ci.wallTime = wallTime;
					}
					ci.hasShuriken = hasAnyProjectileOfTypeStrNCmp(player, "ShurikenObj");  // is linked, but never unlinks, so we just check the projectile
					ci.hasKunaiWall = hasAnyProjectileOfType(player, "Kunai_Wall");
					ci.hasRyuuYanagi = hasAnyProjectileOfType(player, "Kunai");
				} else if (player.charType == CHARACTER_TYPE_SOL) {
					SolInfo& si = currentFrame.u.solInfo;
					si.currentDI = player.playerval1 < 0 ? USHRT_MAX : player.playerval1;
					si.maxDI = player.maxDI;
					
					bool gunflameDisappearsOnHit = false;
					bool gunflameComesOutLater = false;
					bool gunflameFirstWaveDisappearsOnHit = false;
					
					analyzeGunflame(player, &gunflameDisappearsOnHit,
						&gunflameComesOutLater, &gunflameFirstWaveDisappearsOnHit);
					
					si.gunflameDisappearsOnHit = gunflameDisappearsOnHit;
					si.gunflameComesOutLater = gunflameComesOutLater;
					si.gunflameFirstWaveDisappearsOnHit = gunflameFirstWaveDisappearsOnHit;
					
					si.hasTyrantRavePunch2 = hasProjectileOfType(player, "TyrantRavePunch2_DI");
				} else if (player.charType == CHARACTER_TYPE_KY) {
					KyInfo& ki = currentFrame.u.kyInfo;
					ki.stunEdgeWillDisappearOnHit = hasLinkedProjectileOfType(player, "StunEdgeObj");
					ki.hasChargedStunEdge = hasProjectileOfType(player, "ChargedStunEdgeObj");
					ki.hasSPChargedStunEdge = hasProjectileOfType(player, "SPChargedStunEdgeObj");
					ki.hasjD = hasProjectileOfType(player, "AirDustAttackObj");
				} else if (player.charType == CHARACTER_TYPE_ZATO) {
					ZatoInfo& zi = currentFrame.u.zatoInfo;
					zi.currentEddieGauge = player.pawn.exGaugeValue(0);
					zi.maxEddieGauge = 6000;
					// player.eddie.ptr is up to date here
					zi.hasGreatWhite = player.eddie.ptr && strcmp(player.eddie.anim, "EddieMegalithHead") == 0;
					zi.hasInviteHell = hasAnyProjectileOfTypeStrNCmp(player, "Drill");
					zi.hasEddie = player.eddie.ptr;
				} else if (player.charType == CHARACTER_TYPE_SLAYER) {
					SlayerInfo& si = currentFrame.u.slayerInfo;
					si.currentBloodsuckingUniverseBuff = player.wasPlayerval1Idling;
					si.maxBloodsuckingUniverseBuff = player.maxDI;
					si.hasRetro = hasProjectileOfType(player, "Retro");
				} else if (player.charType == CHARACTER_TYPE_INO) {
					InoInfo& ii = currentFrame.u.inoInfo;
					ii.airdashTimer = player.wasProhibitFDTimer;
					ii.noteTime = player.noteTimeWithSlow;
					ii.noteTimeMax = player.noteTimeWithSlowMax;
					ii.noteLevel = player.noteLevel;
					ii.hasChemicalLove = hasLinkedProjectileOfType(player, "BChemiLaser")
						|| hasLinkedProjectileOfType(player, "CChemiLaser");
					ii.hasNote = hasLinkedProjectileOfType(player, "Onpu");
					ii.has5DYRC = hasLinkedProjectileOfType(player, "DustObjShot");
				} else if (player.charType == CHARACTER_TYPE_BEDMAN) {
					fillInBedmanSealInfo(player);
					
					currentFrame.u.bedmanInfo = player.bedmanInfo;
				} else if (player.charType == CHARACTER_TYPE_RAMLETHAL) {
					fillRamlethalDisappearance(currentFrame, player);
				} else if (player.charType == CHARACTER_TYPE_ELPHELT) {
					ElpheltInfo& ei = currentFrame.u.elpheltInfo;
					ei.grenadeTimer = player.wasResource;
					ei.grenadeDisabledTimer = min(255, player.elpheltGrenadeRemainingWithSlow);
					ei.grenadeDisabledTimerMax = min(255, player.elpheltGrenadeMaxWithSlow);
					ei.hasGrenade = elpheltGrenadeExists(player);
					ei.hasJD = elpheltJDExists(player);
				} else if (player.charType == CHARACTER_TYPE_JOHNNY) {
					JohnnyInfo& ji = currentFrame.u.johnnyInfo;
					ji.mistTimer = minmax(0, 1023, player.johnnyMistTimerWithSlow);
					ji.mistTimerMax = minmax(0, 1023, player.johnnyMistTimerMaxWithSlow);
					ji.mistKuttsukuTimer = minmax(0, 1023, player.johnnyMistKuttsukuTimerWithSlow);
					ji.mistKuttsukuTimerMax = minmax(0, 1023, player.johnnyMistKuttsukuTimerMaxWithSlow);
					ji.hasMistKuttsuku = hasAnyProjectileOfType(player, "MistKuttsuku");
					ji.hasMist = hasLinkedProjectileOfType(player, "Mist");
				} else if (player.charType == CHARACTER_TYPE_RAVEN) {
					RavenInfo& ri = currentFrame.u.ravenInfo;
					ri = player.ravenInfo;
				} else if (player.charType == CHARACTER_TYPE_DIZZY) {
					fillDizzyInfo(player, currentFrame);
				} else if (player.charType == CHARACTER_TYPE_MAY) {
					MayInfo& mi = currentFrame.u.mayInfo;
					mi.hasDolphin = hasProjectileOfType(player, "IrukasanRidingObject");
					mi.hasBeachBall = hasProjectileOfTypeStrNCmp(player, "MayBall");
				} else if (player.charType == CHARACTER_TYPE_POTEMKIN) {
					currentFrame.u.potemkinInfo.hasBomb = hasLinkedProjectileOfType(player, "Bomb");  // Trishula
				} else if (player.charType == CHARACTER_TYPE_FAUST) {
					currentFrame.u.faustInfo.hasFlower = hasAnyProjectileOfType(player, "OreHana_Shot")
						|| hasAnyProjectileOfType(player, "OreHanaBig_Shot");
				} else if (player.charType == CHARACTER_TYPE_AXL) {
					AxlInfo& ai = currentFrame.u.axlInfo;
					ai.hasSpindleSpinner = hasProjectileOfType(player, "RashosenObj");
					ai.hasSickleFlash = hasAnyProjectileOfType(player, "RensengekiObj");
					ai.hasMelodyChain = hasAnyProjectileOfType(player, "KyokusagekiObj");
					ai.hasSickleStorm = hasAnyProjectileOfType(player, "ByakueObj");
				} else if (player.charType == CHARACTER_TYPE_VENOM) {
					VenomInfo& vi = currentFrame.u.venomInfo;
					static const char dubiousCurveName[] = "DubiousCurve";
					const char dubiousCurveLetter = player.anim[sizeof dubiousCurveName - 1];
					const bool dubiousCurve = strncmp(player.anim, dubiousCurveName, sizeof dubiousCurveName - 1) == 0;
					const bool hasChangedState = player.pawn.hasUpon(BBSCREVENT_PLAYER_CHANGED_STATE);
					const bool hasQVShockwave = hasProjectileOfType(player, "Debious_AttackBall");
					const bool isFirstFrameOfLackOfChangedState = dubiousCurve
						&& !hasChangedState
						&& player.pawn.bbscrCurrentFunc()
						&& dubiousCurveLetter >= 'A'
						&& dubiousCurveLetter <= 'D'
						&& player.pawn.bbscrCurrentInstr() ==
							player.pawn.bbscrCurrentFunc() + *moves.venomQvClearUponAfterExitOffsetArray[dubiousCurveLetter - 'A']
						&& player.pawn.spriteFrameCounter() == 0;
					
					vi.hasQV = hasQVShockwave
						&& dubiousCurve
						&& hasChangedState;
					
					vi.hasQVYRCOnly = hasQVShockwave && isFirstFrameOfLackOfChangedState;
					vi.hasHCarcassBall = hasHitstunTiedVenomBall(player);
					vi.performingQVA = false;
					vi.performingQVB = false;
					vi.performingQVC = false;
					vi.performingQVD = false;
					vi.performingQVAHitOnly = false;
					vi.performingQVBHitOnly = false;
					vi.performingQVCHitOnly = false;
					vi.performingQVDHitOnly = false;
					if (dubiousCurve && (
							hasChangedState
							|| isFirstFrameOfLackOfChangedState
						)
					) {
						switch (dubiousCurveLetter) {
							case 'A': vi.performingQVA = true; break;
							case 'B': vi.performingQVB = true; break;
							case 'C': vi.performingQVC = true; break;
							case 'D': vi.performingQVD = true; break;
							// here C# would complain about lack of a default case
						}
					}
					if (dubiousCurve && player.pawn.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
						switch (dubiousCurveLetter) {
							case 'A': vi.performingQVAHitOnly = true; break;
							case 'B': vi.performingQVBHitOnly = true; break;
							case 'C': vi.performingQVCHitOnly = true; break;
							case 'D': vi.performingQVDHitOnly = true; break;
							// here C# would complain about lack of a default case
						}
					}
				} else if (player.charType == CHARACTER_TYPE_LEO) {
					LeoInfo& li = currentFrame.u.leoInfo;
					li.hasEdgeyowai = hasLinkedProjectileOfType(player, "Edgeyowai");
					li.hasEdgetuyoi = hasLinkedProjectileOfType(player, "Edgetuyoi");
				} else if (player.charType == CHARACTER_TYPE_JACKO) {
					fillInJackoInfo(player, currentFrame);
				} else if (player.charType == CHARACTER_TYPE_HAEHYUN) {
					HaehyunInfo& hi = currentFrame.u.haehyunInfo;
					
					bool cantDoBall = (player.wasForceDisableFlags & 0x1) != 0 && player.haehyunBallTimeWithSlow != -1;
					hi.cantDoBall = cantDoBall;
					
					if (!cantDoBall) {
						hi.ballTime = minmax(0, USHRT_MAX, player.haehyunBallRemainingTimeWithSlow);
						hi.ballTimeMax = minmax(0, USHRT_MAX, player.haehyunBallRemainingTimeMaxWithSlow);
					} else {
						hi.ballTime = minmax(0, USHRT_MAX, player.haehyunBallTimeWithSlow);
						hi.ballTimeMax = minmax(0, USHRT_MAX, player.haehyunBallTimeMaxWithSlow);
					}
					
					bool ended = false;
					for (int i = 0; i < 2; ++i) {
						if (!ended && player.haehyunSuperBallRemainingTimeWithSlow[i]) {
							hi.superballTime[i].time = minmax(0, USHRT_MAX, player.haehyunSuperBallRemainingTimeWithSlow[i]);
							hi.superballTime[i].timeMax = minmax(0, USHRT_MAX, player.haehyunSuperBallRemainingTimeMaxWithSlow[i]);
						} else {
							ended = true;
							hi.superballTime[i].time = 0;
							hi.superballTime[i].timeMax = 0;
						}
					}
					
					hi.hasBall = hasLinkedProjectileOfType(player, "EnergyBall");
					hi.has5D = hasLinkedProjectileOfType(player, "kum_205shot");
				} else if (player.charType == CHARACTER_TYPE_BAIKEN) {
					BaikenInfo& bi = currentFrame.u.baikenInfo;
					bi.has5D = hasLinkedProjectileOfType(player, "NmlAtk5EShotObj");
					bi.hasJD = hasLinkedProjectileOfType(player, "NmlAtkAir5EShotObj");
					bi.hasTeppou = hasLinkedProjectileOfType(player, "TeppouObj");
					bi.hasTatami = hasLinkedProjectileOfType(player, "TatamiAirObj")
						|| hasLinkedProjectileOfType(player, "TatamiLandObj");
				} else if (player.charType == CHARACTER_TYPE_ANSWER) {
					AnswerInfo& ai = currentFrame.u.answerInfo;
					
					ai.hasCardDestroyOnDamage = false;
					ai.hasCardPlayerGotHit = false;
					ai.hasRSFStart = false;
					ai.hasClone = false;
					for (ProjectileInfo& projectile : currentState->projectiles) {
						if (projectile.team == player.index && projectile.isDangerous && projectile.ptr) {
							if (strcmp(projectile.animName, "Meishi") == 0) {
								if (projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
									ai.hasCardPlayerGotHit = true;
								}
								if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
									ai.hasCardDestroyOnDamage = true;
								}
							} else if (strcmp(projectile.animName, "RSF_Start") == 0) {
								if (projectile.ptr.hasUpon(BBSCREVENT_PLAYER_GOT_HIT)) {
									ai.hasRSFStart = true;
								}
							} else if (strcmp(projectile.animName, "Nin_Jitsu") == 0) {
								if (projectile.ptr.linkObjectDestroyOnDamage() != nullptr) {
									ai.hasClone = true;
								}
							}
						}
					}
					
				} else {
					currentFrame.u.milliaInfo = milliaInfo;
				}
				
				if (player.move.butForFramebarDontCombineWithPreviousMove
						&& !player.startedUp
						&& player.startupProj) {
					if (!player.startupProjIgnoredForFramebar
							&& player.animFrame == 1
							&& !player.pawn.isRCFrozen()
							&& !superflashInstigator) {
						if (!player.activesProj.count || player.activesProj.last().nonActives) {
							player.startupProjIgnoredForFramebar = player.activesProj.count;
						}
						framebar.stateHead->requestFirstFrame = true;
					} else if (player.startupProjIgnoredForFramebar != player.activesProj.count) {
						player.startupProjIgnoredForFramebar = 0;
					}
				}
				if (player.wasHitOnPreviousFrame && inXStun && !player.wakeupTiming) {
					framebar.stateHead->requestFirstFrame = true;
				}
				
				currentFrame.aswEngineTick = aswEngineTickCount;
				if (inXStun) {
					currentFrame.type = standardXStun;
				} else if (!player.idle && player.hitstop && !isAzami) {
					currentFrame.type = hasHitboxes ? FT_ACTIVE_HITSTOP : FT_HITSTOP;
				} else if (!player.idle) {
					if (player.inNewMoveSection && player.move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot) {
						overrideStartupFrame = true;
						defaultStartupFrame = FT_IDLE_ELPHELT_RIFLE;
					} else {
						bool isInVariableStartupSection = player.isInVariableStartupSection();
						bool isInASectionBeforeVariableStartup = false;
						if (isInVariableStartupSection) {
							overrideStartupFrame = true;
							defaultRecoveryFrameType = FT_RECOVERY_HAS_GATLINGS;
							if (player.move.considerVariableStartupAsStanceForFramebar) {
								if (player.move.aSectionBeforeVariableStartup) {
									defaultStartupFrame = FT_STARTUP_ANYTIME_NOW_CAN_ACT;
								} else if (player.move.canStopHolding && player.move.canStopHolding(player)) {
									defaultStartupFrame = FT_STARTUP_STANCE_CAN_STOP_HOLDING;
								} else {
									defaultStartupFrame = FT_STARTUP_STANCE;
								}
							} else if (zatoFrameType != FT_NONE) {
								defaultStartupFrame = zatoFrameType;
							} else {
								defaultStartupFrame = FT_STARTUP_ANYTIME_NOW;
							}
						} else if (player.move.aSectionBeforeVariableStartup) {
							isInASectionBeforeVariableStartup = player.move.aSectionBeforeVariableStartup(player);
							if (isInASectionBeforeVariableStartup) {
								defaultStartupFrame = FT_STARTUP_ANYTIME_NOW;
							}
						} else if (zatoFrameType != FT_NONE) {
							defaultStartupFrame = zatoFrameType;
							overrideStartupFrame = true;
						} else if (milliaInfo.canProgramSecretGarden) {
							overrideStartupFrame = true;
							defaultStartupFrame = FT_STARTUP_CAN_PROGRAM_SECRET_GARDEN;
						}
						if (!isInVariableStartupSection && !isInASectionBeforeVariableStartup && !milliaInfo.canProgramSecretGarden && zatoFrameType == FT_NONE) {
							defaultStartupFrame = startupFrameType;
						}
					}
					currentFrame.type = defaultStartupFrame;
				} else if (!player.canBlock) {
					if (player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime) {
						if (player.inNewMoveSection && player.move.considerNewSectionAsBeingInElpheltRifleStateBeforeBeingAbleToShoot) {
							defaultStartupFrame = FT_IDLE_ELPHELT_RIFLE_READY;
						} else if (player.move.faustPogo) {
							defaultStartupFrame = FT_IDLE_CANT_BLOCK;
						} else {
							defaultStartupFrame = FT_STARTUP_STANCE;
						}
						overrideStartupFrame = true;
						currentFrame.type = defaultStartupFrame;
					} else {
						currentFrame.type = FT_IDLE_CANT_BLOCK;
					}
				} else if (!player.canFaultlessDefense) {
					currentFrame.type = FT_IDLE_CANT_FD;
				} else if (player.airborne
						&& player.idle
						&& player.canBlock
						&& player.y == 0
						&& player.speedY != 0
						&& !player.pawn.ascending()) {  // the last check idk if I need it
					currentFrame.type = FT_IDLE_AIRBORNE_BUT_CAN_GROUND_BLOCK;
				} else if (player.wasCantBackdashTimer != 0) {
					currentFrame.type = FT_BACKDASH_RECOVERY;
				} else if (player.cmnActIndex == CmnActJumpLanding
						&& player.idle && !player.pawn.enableWalkForward()) {
					currentFrame.type = FT_NORMAL_LANDING_RECOVERY;
				} else {
					currentFrame.type = FT_IDLE;
				}
				currentFrame.isFirst = framebar.stateHead->requestFirstFrame;
				currentFrame.newHit = framebar.stateHead->requestNextHit;
				currentFrame.enableNormals = player.wasEnableNormals;
				currentFrame.canBlock = player.canBlock;
				currentFrame.strikeInvulInGeneral = player.strikeInvul.active;
				currentFrame.throwInvulInGeneral = player.throwInvul.active;
				currentFrame.OTGInGeneral = player.wasOtg;
				currentFrame.superArmorActiveInGeneral = player.superArmor.active || player.projectileOnlyInvul.active || player.reflect.active;
				
				#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) currentFrame.fieldName = player.fieldName.active;
				INVUL_TYPES_TABLE
				#undef INVUL_TYPES_EXEC
				
				currentFrame.superArmorActiveInGeneral_IsFull =
					currentFrame.superArmorActiveInGeneral
					&& (
						player.superArmorMid.active
						&& player.superArmorLow.active
						&& player.superArmorOverhead.active
						&& (
							strcmp(player.anim, "CounterGuardStand") == 0
							|| strcmp(player.anim, "CounterGuardCrouch") == 0
							|| strcmp(player.anim, "CounterGuardAir") == 0
						)
						&& player.superArmorHontaiAttacck.active
						&& !player.projectileOnlyInvul.active
						&& !player.reflect.active
						|| player.charType == CHARACTER_TYPE_FAUST
						&& strcmp(player.anim, "NmlAtk5E") == 0
						&& strcmp(player.sprite.name, "fau205_05") == 0
					);
				
				currentFrame.airborne = player.airborne;
				currentFrame.hitAlreadyHappened = hitAlreadyHappened;
				currentFrame.hitConnected = (
						player.pawn.hitSomethingOnThisFrame()
						|| player.hitSomething
					)
					|| player.pawn.inBlockstunNextFrame()
					|| player.pawn.inHitstunNextFrame()
					|| player.armoredHitOnThisFrame
					|| player.gotHitOnThisFrame;
				currentFrame.enableSpecialCancel = enableSpecialCancel;
				currentFrame.clashCancelTimer = player.wasClashCancelTimer;
				currentFrame.enableJumpCancel = player.wasEnableJumpCancel;
				currentFrame.enableSpecials = false;
				player.determineMoveNameAndSlangName(&currentFrame.animName);
				if (prevFrame.cancels && prevFrame.cancels->equalTruncated(player.wasCancels)) {
					currentFrame.cancels = prevFrame.cancels;
				} else if (player.wasCancels.gatlings.empty() && player.wasCancels.whiffCancels.empty() && !player.wasCancels.whiffCancelsNote) {
					currentFrame.cancels = nullptr;
				} else {
					if (!currentFrame.cancels || currentFrame.cancels.use_count() > 1) {
						ThreadUnsafeSharedResource<FrameCancelInfoStored>* newResource = new ThreadUnsafeSharedResource<FrameCancelInfoStored>();
						currentFrame.cancels = ThreadUnsafeSharedPtr<FrameCancelInfoStored>(newResource);
					}
					currentFrame.cancels->copyFromAnotherArray(player.wasCancels);
				}
				currentFrame.dustGatlingTimer = player.dustGatlingTimer;
				currentFrame.dustGatlingTimerMax = player.dustGatlingTimerMax;
				currentFrame.hitstop = player.hitstopWithSlow;
				currentFrame.hitstopMax = player.hitstopMaxWithSlow;
				if (player.stagger && player.cmnActIndex == CmnActJitabataLoop) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = true;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.staggerWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.staggerMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
				} else if (player.blockstun) {
					currentFrame.stop.isBlockstun = true;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					if (player.blockstunContaminatedByRCSlowdown) {
						currentFrame.stop.value = min(player.blockstunWithSlow, 8192);
						currentFrame.stop.valueMax = min(player.blockstunMaxWithSlow, 2047);
						currentFrame.stop.valueMaxExtra = 0;
					} else {
						currentFrame.stop.value = min(player.blockstun, 8192);
						currentFrame.stop.valueMax = min(player.blockstunMax, 2047);
						currentFrame.stop.valueMaxExtra = min(player.blockstunMaxLandExtra, 15);
					}
					currentFrame.stop.tumble = 0;
				} else if (player.hitstun && player.inHitstun) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = true;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					if (player.hitstunContaminatedByRCSlowdown) {
						currentFrame.stop.value = min(player.hitstunWithSlow, 8192);
						currentFrame.stop.valueMax = min(player.hitstunMaxWithSlow, 2047);
						currentFrame.stop.valueMaxExtra = 0;
					} else {
						currentFrame.stop.value = min(player.hitstun, 8192);
						currentFrame.stop.valueMax = min(player.hitstunMax, 2047);
						currentFrame.stop.valueMaxExtra = min(player.hitstunMaxFloorbounceExtra, 15);
					}
					if (player.cmnActIndex == CmnActKorogari) {
						currentFrame.stop.tumble = min(player.tumbleWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.tumbleMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActWallHaritsuki) {
						currentFrame.stop.tumble = min(player.wallstickWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.wallstickMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = true;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActFDownLoop
							|| player.cmnActIndex == CmnActBDownLoop
							|| player.cmnActIndex == CmnActVDownLoop) {
						currentFrame.stop.tumble = min(player.knockdownWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.knockdownMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = true;
					} else {
						currentFrame.stop.tumble = 0;
						currentFrame.stop.tumbleMax = 0;
					}
				} else if (player.wakeupTiming) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = true;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.wakeupTimingWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.wakeupTimingMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else if (player.cmnActIndex == CmnActHajikareStand
						|| player.cmnActIndex == CmnActHajikareCrouch
						|| player.cmnActIndex == CmnActHajikareAir) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = true;
					currentFrame.stop.value = min(player.rejectionWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.rejectionMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else if (player.wallslumpLand && player.cmnActIndex == CmnActWallHaritsukiLand) {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = true;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					currentFrame.stop.value = min(player.wallslumpLandWithSlow, 8192);
					currentFrame.stop.valueMax = min(player.wallslumpLandMaxWithSlow, 2047);
					currentFrame.stop.valueMaxExtra = 0;
					currentFrame.stop.tumble = 0;
					currentFrame.stop.tumbleMax = 0;
				} else {
					currentFrame.stop.isBlockstun = false;
					currentFrame.stop.isHitstun = false;
					currentFrame.stop.isStagger = false;
					currentFrame.stop.isWakeup = false;
					currentFrame.stop.isRejection = false;
					if (player.cmnActIndex == CmnActKorogari) {
						currentFrame.stop.tumble = min(player.tumbleWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.tumbleMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActWallHaritsuki) {
						currentFrame.stop.tumble = min(player.wallstickWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.wallstickMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = true;
						currentFrame.stop.tumbleIsKnockdown = false;
					} else if (player.cmnActIndex == CmnActFDownLoop
							|| player.cmnActIndex == CmnActBDownLoop
							|| player.cmnActIndex == CmnActVDownLoop) {
						currentFrame.stop.tumble = min(player.knockdownWithSlow, 0xffff);
						currentFrame.stop.tumbleMax = min(player.knockdownMaxWithSlow, 0xffff);
						currentFrame.stop.tumbleIsWallstick = false;
						currentFrame.stop.tumbleIsKnockdown = true;
					} else {
						currentFrame.stop.tumble = 0;
						currentFrame.stop.tumbleMax = 0;
					}
				}
				currentFrame.IBdOnThisFrame = player.inBlockstunNextFrame
					&& player.lastBlockWasIB;
				currentFrame.FDdOnThisFrame = player.inBlockstunNextFrame
					&& player.lastBlockWasFD;
				currentFrame.blockedOnThisFrame = player.inBlockstunNextFrame;
				currentFrame.lastBlockWasIB = player.lastBlockWasIB;
				currentFrame.lastBlockWasFD = player.lastBlockWasFD;
				
				if (!player.airborne || player.airborne && player.y == 0 && player.speedY != 0 && !player.pawn.ascending()) {
					int crossupProtection = player.pawn.crossupProtection();
					currentFrame.crossupProtectionIsOdd = (crossupProtection & 1);
					currentFrame.crossupProtectionIsAbove1 = (crossupProtection & 2) >> 1;
					currentFrame.crossedUp = false;
				} else {
					currentFrame.crossupProtectionIsOdd = 0;
					currentFrame.crossupProtectionIsAbove1 = 0;
					currentFrame.crossedUp = player.pawn.crossupProtection() == 3;
				}
				currentFrame.rcSlowdown = player.rcSlowedDownCounter;
				currentFrame.rcSlowdownMax = player.rcSlowedDownMax;
				
				if (player.poisonDuration) {
					currentFrame.poisonDuration = player.poisonDuration;
					currentFrame.poisonMax = 360;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = false;
				} else if (other.charType == CHARACTER_TYPE_JOHNNY) {
					currentFrame.poisonDuration = other.johnnyMistKuttsukuTimerWithSlow;
					currentFrame.poisonMax = other.johnnyMistKuttsukuTimerMaxWithSlow;
					currentFrame.poisonIsBacchusSigh = true;
					currentFrame.poisonIsRavenSlow = false;
				} else if (other.charType == CHARACTER_TYPE_RAVEN) {
					currentFrame.poisonDuration = other.ravenInfo.slowTime;
					currentFrame.poisonMax = other.ravenInfo.slowTimeMax;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = true;
				} else {
					currentFrame.poisonDuration = 0;
					currentFrame.poisonMax = 0;
					currentFrame.poisonIsBacchusSigh = false;
					currentFrame.poisonIsRavenSlow = false;
				}
				
				currentFrame.needShowAirOptions = player.regainedAirOptions && player.airborne;
				currentFrame.doubleJumps = player.remainingDoubleJumps;
				currentFrame.airDashes = player.remainingAirDashes;
				currentFrame.activeDuringSuperfreeze = false;
				
				currentFrame.multipleInputs = player.inputs.size() != 1;
				if (currentFrame.multipleInputs) {
					if (!currentFrame.inputs || currentFrame.inputs.use_count() != 1) {
						currentFrame.inputs = new ThreadUnsafeSharedResource<std::vector<Input>>();
					}
					std::vector<Input>& currentFrameInputs = *currentFrame.inputs;
					currentFrameInputs = player.inputs;
				} else {
					currentFrame.input = player.inputs[0];
				}
				currentFrame.prevInput = player.prevInput;
				currentFrame.inputsOverflow = player.inputsOverflow;
				if (player.inputs.empty()) {
					player.prevInput = Input{0x0000};
				} else {
					player.prevInput = player.inputs.back();
				}
				player.inputsOverflow = false;
				player.inputs.clear();
				
				if (player.move.createdProjectile) {
					const CreatedProjectileStruct* proj = player.move.createdProjectile(player);
					if (proj) {
						if (!currentFrame.createdProjectiles || currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = new ThreadUnsafeSharedResource<std::vector<CreatedProjectileStruct>>();
						} else {
							currentFrame.createdProjectiles->clear();
						}
						currentFrame.createdProjectiles->push_back(*proj);
					} else if (currentFrame.createdProjectiles && !currentFrame.createdProjectiles->empty()) {
						if (currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = nullptr;
						} else {
							currentFrame.createdProjectiles->clear();
						}
					}
				} else if (player.createdProjectiles.empty()) {
					if (currentFrame.createdProjectiles && !currentFrame.createdProjectiles->empty()) {
						if (currentFrame.createdProjectiles.use_count() != 1) {
							currentFrame.createdProjectiles = nullptr;
						} else {
							currentFrame.createdProjectiles->clear();
						}
					}
				} else {
					if (!currentFrame.createdProjectiles || currentFrame.createdProjectiles.use_count() != 1) {
						currentFrame.createdProjectiles = new ThreadUnsafeSharedResource<std::vector<CreatedProjectileStruct>>();
					}
					*currentFrame.createdProjectiles = player.createdProjectiles;
				}
				
				const InputRingBuffer* ringBuffer = game.getInputRingBuffers() + player.index;
				int charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, false);
				currentFrame.chargeLeft = charge > 254 ? 255 : charge;
				if (charge) player.chargeLeftLast = charge;
				currentFrame.chargeLeftLast = player.chargeLeftLast;
				charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_HORIZONTAL, true);
				currentFrame.chargeRight = charge > 254 ? 255 : charge;
				currentFrame.chargeRightLast = player.chargeRightLast;
				if (charge) player.chargeRightLast = charge;
				charge = ringBuffer->parseCharge(InputRingBuffer::CHARGE_TYPE_VERTICAL, false);
				currentFrame.chargeDown = charge > 254 ? 255 : charge;
				if (charge) player.chargeDownLast = charge;
				currentFrame.chargeDownLast = player.chargeDownLast;
				
				static const StringWithLength elpheltShotgunFire = "Shotgun_Fire_";
				currentFrame.powerup = player.move.powerup ? player.move.powerup(player) : nullptr;
				if (currentFrame.powerup) {
					currentFrame.dontShowPowerupGraphic = player.move.dontShowPowerupGraphic ? player.move.dontShowPowerupGraphic(player) : false;
				} else if (player.charType == CHARACTER_TYPE_MILLIA && player.pickedUpSilentForceKnifeOnThisFrame) {
					currentFrame.powerup = "Picked up Silent Force";
					currentFrame.dontShowPowerupGraphic = false;
				} else if (player.charType == CHARACTER_TYPE_ELPHELT
						&& player.playerval0 && !player.prevFramePlayerval1 && player.playerval1
						&& (
							player.cmnActIndex == CmnActCrouch2Stand
							|| player.cmnActIndex == CmnActStand
							|| player.cmnActIndex == NotACmnAct
							&& strncmp(player.anim, elpheltShotgunFire.txt, elpheltShotgunFire.length) == 0
						)) {
					currentFrame.powerup = "Ms. Travailler reached maximum charge.";
					currentFrame.dontShowPowerupGraphic = false;
				} else {
					currentFrame.dontShowPowerupGraphic = false;
				}
				currentFrame.cantAirdash = player.airborne && player.wasCantAirdash && !player.inHitstun;
				currentFrame.airthrowDisabled = player.airborne && player.pawn.airthrowDisabled();
				currentFrame.running = player.pawn.running()
					|| player.cmnActIndex == CmnActFDash
					&& (
						player.pawn.isDisableThrow()  // fix for Johnny's dash
						|| player.charType == CHARACTER_TYPE_LEO  // Leo's is a special move with whiff cancels, but still we must make it very clear he cannot throw
						// For Bedman and Slayer it's kind of obvious they can't do anything at all during their dash, not just throw
					);
				currentFrame.cantBackdash = player.wasCantBackdashTimer != 0;
				if (player.x == player.prevPosX) {
					currentFrame.suddenlyTeleported = false;
				} else if (player.x > player.prevPosX) {
					currentFrame.suddenlyTeleported = player.x - player.prevPosX >= player.speedX + 122499;
				} else {
					currentFrame.suddenlyTeleported = player.prevPosX - player.x >= -player.speedX + 122499;
				}
				if (!currentFrame.suddenlyTeleported && player.y != player.prevPosY) {
					if (player.y > player.prevPosY) {
						currentFrame.suddenlyTeleported = player.y - player.prevPosY >= player.speedY + 122499;
					} else {
						currentFrame.suddenlyTeleported = player.prevPosY - player.y >= -player.speedY + 122499;
					}
				}
				
				currentFrame.counterhit = player.counterhit;
				currentFrame.crouching = player.crouching;
				
			} else if (superflashInstigator && player.gotHitOnThisFrame) {
				currentFrame.hitConnected = true;
			}
			
			FrameType recoveryFrameType = defaultRecoveryFrameType;
			bool overridePrevRecoveryFrames = false;
			if (hasCancelsRecoveryFrameType != FT_NONE) {
				recoveryFrameType = hasCancelsRecoveryFrameType;
				overridePrevRecoveryFrames = hasCancelsOverridePrevRecovery;
			}
			
			if (player.cmnActIndex == CmnActRomanCancel && !player.superfreezeStartup) {
				recoveryFrameType = FT_STARTUP;
			} else if (player.move.secondaryStartup && player.move.secondaryStartup(player)) {
				recoveryFrameType = FT_STARTUP;
				if (prevFrame.type != FT_NONE && prevFrame.type == FT_RECOVERY_HAS_GATLINGS) {
					framebar.stateHead->requestFirstFrame = true;
					if (framebarAdvancedIdleHitstop) {
						currentFrame.isFirst = true;
					}
				}
			}
			
			if (inXStun) {
				startupFrameType = standardXStun;
			} else if (!player.startedUp
					&& (player.startupProj && !player.startupProjIgnoredForFramebar)
					&& !(
						player.cmnActIndex == CmnActRomanCancel
						&& !player.superfreezeStartup
					)
					&& !(
						player.charType == CHARACTER_TYPE_FAUST
						&& strcmp(player.anim, "NmlAtk5E") == 0
					)) {
				if (hasCancels && recoveryFrameType == FT_RECOVERY) {
					startupFrameType = FT_RECOVERY_HAS_GATLINGS;
				} else {
					startupFrameType = recoveryFrameType;
				}
			} else if (player.cmnActIndex == CmnActRomanCancel && player.superfreezeStartup) {
				startupFrameType = recoveryFrameType;
			} else if (overrideStartupFrame) {
				startupFrameType = defaultStartupFrame;
			}
			
			bool measureInvuls = !(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& (player.strikeInvul.active
						|| player.throwInvul.active && !player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime  // anti Faust-Pogo infinite throw invul
						|| player.projectileOnlyInvul.active
						|| player.superArmor.active
						|| player.reflect.active);
			
			int playerMaxThing = player.total;
			if (player.totalCanBlock > playerMaxThing) playerMaxThing = player.totalCanBlock;
			if (player.totalCanFD > playerMaxThing) playerMaxThing = player.totalCanFD;
			
			if (player.charType == CHARACTER_TYPE_SIN && !player.sinHunger) {
				bool newVal = strcmp(player.pawn.gotoLabelRequests(), "Kuuhuku") == 0 && !player.pawn.isRCFrozen();
				if (newVal) {
					player.sinHunger = true;
					player.determineMoveNameAndSlangName(&currentFrame.animName);
					framebar.stateHead->requestFirstFrame = true;
					currentFrame.isFirst = true;
				}
			}
			
			if (player.sinHunger) {
				currentFrame.type = FT_RECOVERY;
				++player.sinHungerRecovery;
			} else if (player.isInFDWithoutBlockstun) {
				++player.totalFD;
				currentFrame.type = standardXStun;
			} else if (!(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& (
						player.cmnActIndex == CmnActLandingStiff && !player.idle  // Ramlethal j.8D becomes "idle" on f6 of stiff landing
						|| !player.idle
						&& !player.airborne
						&& player.moveOriginatedInTheAir
					)
					&& !hasHitboxes
					&& !player.onTheDefensive  // Potemkin I.C.P.M may cause a situation where, on hit, opponent is put into CmnActLockWait that begins airborne,
					                           // then transitions to the ground and, if this check wasn't here, it would be seen as landing animation (lol)
					&& player.cmnActIndex != CmnActRomanCancel  // needed for Elphelt j.D YRC
				) {
				currentFrame.type = hasCancels ? FT_LANDING_RECOVERY_CAN_CANCEL : FT_LANDING_RECOVERY;
				++player.landingRecovery;
				player.totalFD = 0;
				measureInvuls = player.total
					&& !player.theAnimationIsNotOverYetLolConsiderBusyNonAirborneFramesAsLandingAnimation
					&& !player.ignoreNextInabilityToBlockOrAttack;
			} else if (!(player.hitstop && !isAzami)
					&& !(superflashInstigator && superflashInstigator != ent)
					&& !(player.cmnActIndex == CmnActJump && player.canFaultlessDefense && !player.performingBDC)
					&& (player.cmnActIndex != CmnActJumpPre || player.performingBDC)
					&& !player.isLanding) {
				if (
						(
							(
								!player.idlePlus
								|| player.idleInNewSection
								|| player.forceBusy
								|| !(player.canBlock && player.canFaultlessDefense || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)
							)
							&& !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !player.startedUp
						&& !superflashInstigator
					) {
					if (!player.idlePlus || player.idleInNewSection || player.forceBusy) {
						if (player.landingRecovery) {  // needed for Zato Shadow Gallery. Removing airborne check for Answer s.S
							entityFramebar.changePreviousFrames(landingRecoveryTypes,
								_countof(landingRecoveryTypes),
								startupFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.landingRecovery,
								framebarPos,
								cs->framebarPositionHitstop,
								framebarPosNoHitstop,
								cs->framebarPosition,
								cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
								cs->framebarTotalFramesHitstopUnlimited,
								cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
								cs->framebarTotalFramesUnlimited);
							player.startup += player.landingRecovery;
							player.total = playerMaxThing;
							player.totalCanBlock = playerMaxThing;
							player.totalCanFD = playerMaxThing;
							player.total += player.landingRecovery;
							player.totalCanBlock += player.landingRecovery;
							player.totalCanFD += player.landingRecovery;
							playerMaxThing += player.landingRecovery;
							player.landingRecovery = 0;
						}
						++player.startup;
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						measureInvuls = true;
					}
					if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
						++player.totalCanBlock;
					}
					if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
						++player.totalCanFD;
					}
					if (!player.idlePlus || player.idleInNewSection || player.forceBusy
							|| (
								player.canBlock && player.canFaultlessDefense
								|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
							)) {
						currentFrame.type = startupFrameType;
					}
				}
				if (superflashInstigator == ent && !player.superfreezeStartup) {
					player.superfreezeStartup = player.total;
				}
				if (hasHitboxes) {
					if (!player.startedUp) {
						player.totalCanBlock = player.total;  // needed for Raven glide
						player.totalCanFD = player.total;  // needed for Raven glide
						if (player.startupProj) {
							entityFramebar.changePreviousFramesOneType(FT_RECOVERY,
								startupFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.startup - player.startupProj,
								framebarPos,
								cs->framebarPositionHitstop,
								framebarPosNoHitstop,
								cs->framebarPosition,
								cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
								cs->framebarTotalFramesHitstopUnlimited,
								cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
								cs->framebarTotalFramesUnlimited,
								true);
						}
						player.startedUp = true;
						player.inNewMoveSection = false;
						if (player.charType == CHARACTER_TYPE_POTEMKIN
								&& strcmp(player.anim, "HammerFall") == 0) {
							player.removeNonStancePrevStartups();
						}
						player.addActiveFrame(ent, framebar);
						player.maxHitUse = player.maxHit;
						currentFrame.type = defaultActiveFrame;
						if (superflashInstigator && !framebarAdvancedIdleHitstop && currentFrame.type != FT_NONE) {
							currentFrame.activeDuringSuperfreeze = true;
							if (player.pawn.hitSomethingOnThisFrame()
									|| player.pawn.dealtAttack()->attackMultiHit() && player.hitSomething) {
								currentFrame.hitConnected = true;
							}
						}
					} else if (!superflashInstigator) {
						if (player.recovery + player.landingRecovery) {
							if (player.landingRecovery) {
								entityFramebar.changePreviousFrames(landingRecoveryTypes,  // needed for Venom H Mad Struggle
									_countof(landingRecoveryTypes),
									FT_NON_ACTIVE,
									framebarPos - 1,
									framebarHitstopSearchPos,
									framebarIdleSearchPos,
									framebarSearchPos,
									player.landingRecovery,
									framebarPos,
									cs->framebarPositionHitstop,
									framebarPosNoHitstop,
									cs->framebarPosition,
									cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
									cs->framebarTotalFramesHitstopUnlimited,
									cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
									cs->framebarTotalFramesUnlimited);
								player.recovery += player.landingRecovery;
								player.total = playerMaxThing;
								player.totalCanBlock = playerMaxThing;
								player.totalCanFD = playerMaxThing;
								player.total += player.landingRecovery;
								player.totalCanBlock += player.landingRecovery;
								player.totalCanFD += player.landingRecovery;
								playerMaxThing += player.landingRecovery;
								player.landingRecovery = 0;
							}
							
							entityFramebar.changePreviousFrames(recoveryFrameTypes,
								_countof(recoveryFrameTypes),
								FT_NON_ACTIVE,
								framebarPos - 1 - player.landingRecovery,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								player.recovery,
								framebarPos,
								cs->framebarPositionHitstop,
								framebarPosNoHitstop,
								cs->framebarPosition,
								cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
								cs->framebarTotalFramesHitstopUnlimited,
								cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
								cs->framebarTotalFramesUnlimited);
							
							player.actives.addNonActive(player.recovery + player.landingRecovery);
							player.recovery = 0;
							player.landingRecovery = 0;
						}
						player.addActiveFrame(ent, framebar);
						player.maxHitUse = player.maxHit;
						currentFrame.type = defaultActiveFrame;
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
								&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
							if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
							++player.totalCanBlock;
						}
						if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
								&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
							if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
							++player.totalCanFD;
						}
						measureInvuls = true;
					}
				} else if (
						(
							(
								!player.idlePlus
								|| !(player.canBlock && player.canFaultlessDefense || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)
							) && !player.ignoreNextInabilityToBlockOrAttack
						)
						&& !superflashInstigator
						&& player.startedUp
					) {
					if (player.landingRecovery && !player.idlePlus) {
						entityFramebar.changePreviousFrames(landingRecoveryTypes,
							_countof(landingRecoveryTypes),
							recoveryFrameType,
							framebarPos - 1,
							framebarHitstopSearchPos,
							framebarIdleSearchPos,
							framebarSearchPos,
							player.landingRecovery,
							framebarPos,
							cs->framebarPositionHitstop,
							framebarPosNoHitstop,
							cs->framebarPosition,
							cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
							cs->framebarTotalFramesHitstopUnlimited,
							cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
							cs->framebarTotalFramesUnlimited);
						player.recovery += player.landingRecovery;
						player.total += player.landingRecovery;
						player.totalCanBlock += player.landingRecovery;
						player.totalCanFD += player.landingRecovery;
						player.landingRecovery = 0;
					}
					DWORD recoveryMode = 0;
					if (!player.idlePlus) {
						recoveryMode |= 1;
					}
					if (!player.canBlock && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						recoveryMode |= 2;
					}
					if ((!player.canBlock || !player.canFaultlessDefense) && !player.ignoreNextInabilityToBlockOrAttack
							&& !(player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
						recoveryMode |= 4;
					}
					if (recoveryMode
							&& overridePrevRecoveryFrames
							&& player.recovery == 1) {
						// needed for whiff 5P to not show first recovery frame as cancellable (it is not)
						const FrameCancelInfoStored* cancelInfo = prevFrame.cancels.get();
						if (cancelInfo
								&& (
									!cancelInfo->gatlings.empty()
									|| !cancelInfo->whiffCancels.empty()
								)
								|| prevFrame.enableSpecialCancel  // for Ky 3H
								|| prevFrame.enableSpecials) {
							entityFramebar.changePreviousFramesOneType(FT_RECOVERY,
								recoveryFrameType,
								framebarPos - 1,
								framebarHitstopSearchPos,
								framebarIdleSearchPos,
								framebarSearchPos,
								1,
								framebarPos,
								cs->framebarPositionHitstop,
								framebarPosNoHitstop,
								cs->framebarPosition,
								cs->framebarTotalFramesHitstopUnlimited + cs->framebarIdleHitstopFor,
								cs->framebarTotalFramesHitstopUnlimited,
								cs->framebarTotalFramesUnlimited + cs->framebarIdleFor,
								cs->framebarTotalFramesUnlimited);
						}
					}
					if ((recoveryMode & 1) != 0) {
						if (player.total < playerMaxThing) player.total = playerMaxThing;
						++player.total;
						++player.recovery;
						currentFrame.type = recoveryFrameType;
						measureInvuls = true;
					}
					if ((recoveryMode & 2) != 0) {
						currentFrame.type = recoveryFrameType;
						if (player.totalCanBlock < playerMaxThing) player.totalCanBlock = playerMaxThing;
						++player.totalCanBlock;
					}
					if ((recoveryMode & 4) != 0) {
						currentFrame.type = recoveryFrameType;
						if (player.totalCanFD < playerMaxThing) player.totalCanFD = playerMaxThing;
						++player.totalCanFD;
					}
				}
			}
			if (framebarAdvancedIdleHitstop && (
					frameTypeAssumesCantAttack(currentFrame.type)
					|| player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime
				)) {
				currentFrame.enableSpecials = player.wasEnableSpecials;
			}
			if (framebarAdvancedIdleHitstop) {
				currentFrame.newHit = framebar.stateHead->requestNextHit;
			}
			if (!(player.hitstop && !isAzami)
					&& !superflashInstigator
					&& !(player.cmnActIndex == CmnActJump && player.canFaultlessDefense && !player.performingBDC)
					&& (player.cmnActIndex != CmnActJumpPre || player.performingBDC)
					&& !player.isLanding
					&& (!player.ignoreNextInabilityToBlockOrAttack || player.move.canBeUnableToBlockIndefinitelyOrForVeryLongTime)) {
				PlayerCancelInfo newCancelInfo;
				++player.cancelsTimer;
				newCancelInfo.start = player.cancelsTimer;
				newCancelInfo.end = player.cancelsTimer;
				newCancelInfo.enableSpecialCancel = enableSpecialCancel;
				newCancelInfo.clashCancelTimer = player.wasClashCancelTimer;
				newCancelInfo.enableJumpCancel = player.wasEnableJumpCancel;
				newCancelInfo.copyFromAnotherArray(player.wasCancels);
				newCancelInfo.enableSpecials = false;
				newCancelInfo.hitAlreadyHappened = hitAlreadyHappened;
				newCancelInfo.airborne = player.airborne;
				if (settings.showYrcWindowsInCancelsPanel) {
					newCancelInfo.canYrc = player.canYrcProjectile
						? player.canYrcProjectile
						: player.wasCanYrc
							? (const char*)1
							: nullptr;
				} else {
					newCancelInfo.canYrc = nullptr;
				}
				player.appendPlayerCancelInfo(newCancelInfo);
			}
			
			if (!(player.hitstop || superflashInstigator)) {
				if (measureInvuls) {
					if (!player.stoppedMeasuringInvuls) {
						++player.totalForInvul;
						int prevTotal = player.prevStartups.total() + player.totalForInvul;
						
						#define INVUL_TYPES_EXEC(enumName, stringDesc, fieldName) player.fieldName.addInvulFrame(prevTotal);
						INVUL_TYPES_TABLE
						#undef INVUL_TYPES_EXEC
					}
					
				} else {
					player.stoppedMeasuringInvuls = true;
				}
			}
		}
		
		for (PlayerInfo& player : cs->players) {
			player.startupDisp = 0;
			player.activesDisp.clear();
			player.maxHitDisp.clear();
			player.recoveryDisp = 0;
			player.recoveryDispCanBlock = -1;
			player.totalDisp = 0;
			player.prevStartupsDisp.clear();
			player.prevStartupsTotalDisp.clear();
			player.hitOnFrameDisp;
			if (player.startedUp && !player.startupProj) {
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				player.startupDisp = player.startup;
				player.activesDisp = player.actives;
				player.maxHitDisp = player.maxHitUse;
				player.recoveryDisp = player.recovery;
				player.totalDisp = player.total;
				player.hitOnFrameDisp = player.hitOnFrame;
			} else if (player.startupProj && !player.startedUp) {
				int endOfActivesRelativeToPlayerTotalCountdownStart;
				if (!player.prevStartupsProj.empty()) {
					player.startupDisp = player.startupProj;
					player.prevStartupsDisp = player.prevStartupsProj;
					endOfActivesRelativeToPlayerTotalCountdownStart = player.prevStartupsProj.total() + player.startupProj + player.activesProj.total() - 1;
				} else {
					int prevStartupsTotal = player.prevStartups.total();
					if (player.startupProj < prevStartupsTotal) {  // needed for Dizzy 421H
						player.startupDisp = player.startupProj;
					} else {
						player.startupDisp = player.startupProj - prevStartupsTotal;
						player.prevStartupsDisp = player.prevStartups;
					}
					player.prevStartupsTotalDisp = player.prevStartups;
					endOfActivesRelativeToPlayerTotalCountdownStart = player.startupDisp + player.activesProj.total() - 1;
				}
				player.maxHitDisp = player.maxHitProj; 
				player.hitOnFrameDisp = !player.maxHitProj.empty() && player.maxHitProj.maxUse <= 1 && !player.maxHitProjConflict ? 0 : player.hitOnFrameProj;
				player.activesDisp = player.activesProj;
				int activesDispTotal = player.activesDisp.total();
				if (endOfActivesRelativeToPlayerTotalCountdownStart >= player.total) {
					player.recoveryDisp = 0;
					if (player.totalCanBlock > player.total && endOfActivesRelativeToPlayerTotalCountdownStart < player.totalCanBlock) {
						// needed for Answer Taunt
						player.recoveryDispCanBlock = player.totalCanBlock - endOfActivesRelativeToPlayerTotalCountdownStart;
					}
				} else {
					player.recoveryDisp = player.total - endOfActivesRelativeToPlayerTotalCountdownStart;
				}
				player.totalDisp = player.total;
			} else if (player.startedUp && player.startupProj) {
				if (!player.prevStartupsProj.empty()) {
					int projPrevTotal = player.prevStartupsProj.total();
					player.startupProj += projPrevTotal;
					player.prevStartupsProj.clear();
				}
				if (player.hitOnFrame && player.hitOnFrameProj) {
					if (player.hitOnFrame < player.hitOnFrameProj) {
						player.hitOnFrameDisp = player.hitOnFrame;
					} else {
						player.hitOnFrameDisp = player.hitOnFrameProj;
					}
				} else if (player.hitOnFrame) {
					player.hitOnFrameDisp = player.hitOnFrame;
				} else if (player.hitOnFrameProj) {
					player.hitOnFrameDisp = player.hitOnFrameProj;
				}
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				if (player.startup <= player.startupProj) {
					player.startupDisp = player.startup;
					player.activesDisp = player.actives;
					player.activesDisp.mergeTimeline(player.startupProj - player.startup + 1 - player.prevStartupsDisp.total(), player.activesProj);
				} else {
					player.startupDisp = player.startupProj;
					player.activesDisp = player.activesProj;
					player.activesDisp.mergeTimeline(player.startup - player.startupProj + 1 - player.prevStartupsDisp.total(), player.actives);
				}
				int activesDispTotal = player.activesDisp.total();
				int totalAbsoluteStartup = player.startupDisp + activesDispTotal - 1;
				if (totalAbsoluteStartup >= player.total) {
					player.recoveryDisp = 0;
				} else {
					player.recoveryDisp = player.total - totalAbsoluteStartup;
				}
				player.totalDisp = player.total;
			} else {
				player.prevStartupsDisp = player.prevStartups;
				player.prevStartupsTotalDisp = player.prevStartups;
				player.totalDisp = player.total;
			}
			
			PlayerFramebars& entityFramebar = playerFramebars[player.index];
			PlayerFramebar& framebar = entityFramebar.idleHitstop;
			PlayerFrame& currentFrame = framebar[framebarPos];
			
			int value;
			
			value = player.printStartupForFramebar();
			currentFrame.startup = value > SHRT_MAX ? SHRT_MAX : value;
			
			value = player.activesDisp.total();
			currentFrame.active = value > SHRT_MAX ? SHRT_MAX : value;
			
			value = player.printRecoveryForFramebar();
			currentFrame.recovery = value > SHRT_MAX ? SHRT_MAX : value;
			
			FrameAdvantageForFramebarResult advRes;
			player.calcFrameAdvantageForFramebar(&advRes);
			currentFrame.frameAdvantage = advRes.frameAdvantage;
			currentFrame.landingFrameAdvantage = advRes.landingFrameAdvantage;
			currentFrame.frameAdvantageNoPreBlockstun = advRes.frameAdvantageNoPreBlockstun;
			currentFrame.landingFrameAdvantageNoPreBlockstun = advRes.landingFrameAdvantageNoPreBlockstun;
			
			value = player.totalDisp + player.sinHungerRecovery;
			currentFrame.total = value > SHRT_MAX ? SHRT_MAX : value;
			
			copyIdleHitstopFrameToTheRestOfSubframebars(entityFramebar,
				framebarAdvanced,
				framebarAdvancedIdle,
				framebarAdvancedHitstop,
				framebarAdvancedIdleHitstop);
			
			if (player.hitstop) {
				player.displayHitstop = true;
			}
			if (player.stagger && player.cmnActIndex == CmnActJitabataLoop) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_STAGGER_WITH_SLOW;
			} else if (player.hitstun && player.inHitstun) {
				if (player.rcSlowedDown || player.hitstunContaminatedByRCSlowdown) {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT_WITH_SLOW;
				} else {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_HIT;
				}
			} else if (player.blockstun) {
				if (player.rcSlowedDown || player.blockstunContaminatedByRCSlowdown) {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK_WITH_SLOW;
				} else {
					player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_BLOCK;
				}
			} else if (player.rejection) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_REJECTION_WITH_SLOW;
			} else if (player.cmnActIndex == CmnActWallHaritsukiLand) {
				player.xStunDisplay = PlayerInfo::XSTUN_DISPLAY_WALLSLUMP_LAND;
			}
			player.prevGettingHitBySuper = player.gettingHitBySuper;
			player.prevFrameMem45 = player.pawn.mem45();
			player.prevFrameMem46 = player.pawn.mem46();
			player.prevFrameGroundHitEffect = player.pawn.inflicted()->groundHitEffect;
			player.prevFrameGroundBounceCount = player.pawn.inflicted()->groundBounceCount;
			player.prevFrameTumbleDuration = player.pawn.inflicted()->tumbleDuration;
			player.prevFrameMaxHit = player.pawn.maxHit();
			player.prevFramePlayerval0 = player.playerval0;
			player.prevFramePlayerval1 = player.playerval1;
			player.prevFrameElpheltRifle_AimMem46 = player.elpheltRifle_AimMem46;
			player.prevFrameRomanCancelAvailability = player.pawn.romanCancelAvailability();
			player.prevBbscrvar5 = player.pawn.bbscrvar5();
			for (int k = 0; k < 4; ++k) {
				player.prevFrameResource[k] = player.pawn.exGaugeValue(k);
			}
			player.prevPosX = player.x;
			player.prevPosY = player.y;
			player.prevFrameCancels = player.wasCancels;
			player.prevFramePreviousEntityLinkObjectDestroyOnStateChangeWasEqualToPlayer = player.pawn.previousEntity()
				&& player.pawn.previousEntity().linkObjectDestroyOnStateChange() == player.pawn;
			
		}
		
		for (ThreadUnsafeSharedPtr<ProjectileFramebar>& entityFramebar : projectileFramebars) {
			if (!(
				aswEngineTickCount >= entityFramebar->creationTick && aswEngineTickCount < entityFramebar->deletionTick
			)) {
				continue;
			}
			entityFramebar->main.stateHead->completelyEmpty = entityFramebar->main.lastNFramesCompletelyEmpty(cs->framebarPosition, FRAMES_MAX);
			entityFramebar->hitstop.stateHead->completelyEmpty = entityFramebar->main.lastNFramesCompletelyEmpty(cs->framebarPositionHitstop, FRAMES_MAX);
			
			if (entityFramebar->main.stateHead->completelyEmpty && entityFramebar->hitstop.stateHead->completelyEmpty) {
				entityFramebar->deletionTick = aswEngineTickCount;
			}
		}
	}
	if (frameHasChanged) {
		Entity superflashInstigator = getSuperflashInstigator();
		if (!superflashInstigator) {
			cs->superfreezeHasBeenGoingFor = 0;
		} else {
			cs->lastNonZeroSuperflashInstigator = superflashInstigator;
			++cs->superfreezeHasBeenGoingFor;
		}
	}
	
#ifdef LOG_PATH
	didWriteOnce = true;
#endif
}
