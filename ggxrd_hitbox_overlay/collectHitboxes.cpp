#include "pch.h"
#include "collectHitboxes.h"
#include "logging.h"
#include "pi.h"
#include "EntityList.h"
#include "Hitbox.h"
#include "colors.h"
#include "Hardcode.h"
#include "Moves.h"
#include "Settings.h"
#include "findMoveByName.h"
#include "GifMode.h"

static void getMahojinDistXY(BYTE* functionStart, int* x, int* y);
static void getMayBallJumpConnectOffsetYAndRange(BYTE* functionStart, int* mayBallJumpConnectPtr, int* mayBallJumpConnectRangePtr);

void collectHitboxes(Entity ent,
		const bool active,
		DrawHitboxArrayCallParams* const hurtbox,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
		std::vector<DrawLineCallParams>* const lines,
		std::vector<DrawCircleCallParams>* const circles,
		std::vector<DrawBoxCallParams>* const pushboxes,
		std::vector<DrawBoxCallParams>* const interactionBoxes,
		int* numHitboxes,
		int* lastIgnoredHitNum,
		EntityState* entityState,
		bool* wasSuperArmorEnabled,
		bool* wasFullInvul,
		int scaleX,
		int scaleY) {
	
	CharacterType ownerType = (CharacterType)-1;
	Entity owner { nullptr };
	if (!ent.isPawn()
			&& (ent.team() == 0 || ent.team() == 1)) {
		owner = entityList.slots[ent.team()];
		ownerType = owner.characterType();
	}
	const char* animName = ent.animationName();
	if (ownerType == CHARACTER_TYPE_JACKO
			&& (
				!ent.displayModel()
				|| strncmp(animName, "Ghost", 5) == 0  // "GhostADummy"_hardcode, "GhostBDummy"_hardcode, "GhostCDummy"_hardcode
				&& strncmp(animName + 6, "Dummy", 6) == 0  // "GhostADummy"_hardcode, "GhostBDummy"_hardcode, "GhostCDummy"_hardcode
			)
			|| ent.y() < -3000000  // needed for May [2]8S/H
			|| ent.isHidden()  // needed for super animations
			|| ownerType == CHARACTER_TYPE_LEO
			&& strcmp(animName, "Semuke5E_Reflect"_hardcode) == 0
			|| ownerType == CHARACTER_TYPE_BEDMAN
			&& (
				memcmp(animName, "Djavu_", 6) == 0
				&& memcmp(animName + 7, "_Ghost", 6) == 0
				&& animName[13] == '\0'  // "Djavu_A_Ghost"_hardcode, "Djavu_B_Ghost"_hardcode, "Djavu_C_Ghost"_hardcode, "Djavu_D_Ghost"_hardcode
			)) {
		return;
	}
	
	EntityState state;
	ent.getState(&state, wasSuperArmorEnabled, wasFullInvul);
	if (entityState) *entityState = state;
	
	int currentHitNum = ent.currentHitNum();
	
	DrawHitboxArrayParams params;
	params.posX = state.posX;
	params.posY = state.posY;
	logOnce(fprintf(logfile, "pox_x: %d\n", params.posX));
	logOnce(fprintf(logfile, "pox_y: %d\n", params.posY));
	params.flip = ent.isFacingLeft() ? 1 : -1;
	logOnce(fprintf(logfile, "flip: %d\n", (int)params.flip));

	// Color scheme:
	// light blue - hurtbox on counterhit
	// green - hurtbox on not counterhit
	// red - hitbox
	// yellow - pushbox
	// blue/purple - throwbox

	bool isNotZeroScaled = *(int*)(ent + 0x2594) != 0 || scaleX != INT_MAX;

	if (pushboxes && (
			!state.isASummon
			|| ownerType == CHARACTER_TYPE_JACKO
			&& settings.showJackoSummonsPushboxes
			&& (ent.servant() || ent.ghost())
	) && isNotZeroScaled) {
		logOnce(fputs("Need pushbox\n", logfile));
		// Draw pushbox and throw box
		/*if (is_push_active(asw_data))
		{*/
		int pushboxTop = ent.pushboxTop();
		int pushboxBottom = ent.pushboxBottom();
		
		if (state.isASummon && pushboxTop == 100 && pushboxBottom == 0) {
			pushboxTop = 20000;
		}
		
		DrawBoxCallParams pushboxParams;
		ent.pushboxLeftRight(&pushboxParams.left, &pushboxParams.right);
		pushboxParams.top = params.posY + pushboxTop;
		pushboxParams.bottom = params.posY - pushboxBottom;
		pushboxParams.fillColor = replaceAlpha(state.throwInvuln ? 0 : state.isASummon ? 16 : 64, COLOR_PUSHBOX);
		pushboxParams.outlineColor = replaceAlpha(255, COLOR_PUSHBOX);
		pushboxParams.thickness = THICKNESS_PUSHBOX;
		pushboxParams.hatched = false;
		pushboxParams.originX = params.posX;
		pushboxParams.originY = params.posY;
		pushboxes->push_back(pushboxParams);
		logOnce(fputs("pushboxes->push_back(...) called\n", logfile));
		//}
	}
	
	static const HitboxType overrideHurtboxType = HITBOXTYPE_HURTBOX;
	
	const Hitbox* const hurtboxData = ent.hitboxData(overrideHurtboxType);
	const Hitbox* const hitboxData = ent.hitboxData(HITBOXTYPE_HITBOX);
	
	const int hurtboxCount = ent.hitboxCount(overrideHurtboxType);
	const int hitboxCount = ent.hitboxCount(HITBOXTYPE_HITBOX);

	logOnce(fprintf(logfile, "hurtbox_count: %d; hitbox_count: %d\n", hurtboxCount, hitboxCount));

	// Thanks to jedpossum on dustloop for these offsets
	params.scaleX = scaleX == INT_MAX ? ent.scaleX() : scaleX;
	params.scaleY = scaleY == INT_MAX ? ent.scaleY() : scaleY;
	logOnce(fprintf(logfile, "scale_x: %d; scale_y: %d\n", *(int*)(ent + 0x264), *(int*)(ent + 0x268)));
	
	DrawHitboxArrayCallParams callParams;
	
	if (hurtbox && isNotZeroScaled && hurtboxCount && !(
			ownerType == CHARACTER_TYPE_ELPHELT
			&& !(ent.displayModel() && strcmp(ent.animationName(), "GrenadeBomb_Ready") != 0 || !state.strikeInvuln)
		)) {
		callParams.thickness = THICKNESS_HURTBOX;
		callParams.hitboxData = hurtboxData;
		callParams.hitboxCount = hurtboxCount;
		callParams.params = params;
		callParams.params.angle = ent.pitch();
		callParams.params.hitboxOffsetX = ent.hitboxOffsetX();
		callParams.params.hitboxOffsetY = ent.hitboxOffsetY();
		DWORD alpha = 64;
		if (state.strikeInvuln) {
			alpha = 0;
		}
		if (THICKNESS_WAY_TOO_NUMEROUS_SUMMONS
				&& state.isASummon && (state.ownerCharType == CHARACTER_TYPE_JACKO || state.ownerCharType == CHARACTER_TYPE_BEDMAN)) {
			if (state.ownerCharType == CHARACTER_TYPE_BEDMAN) {
				if (alpha > 48) alpha = 48;
			} else {
				if (alpha > 32) alpha = 32;
			}
			callParams.thickness = THICKNESS_WAY_TOO_NUMEROUS_SUMMONS;
		}
		if (state.counterhit) {
			callParams.thickness = THICKNESS_HURTBOX_COUNTERHIT;
			callParams.fillColor = replaceAlpha(alpha, COLOR_HURTBOX_COUNTERHIT);
			callParams.outlineColor = replaceAlpha(255, COLOR_HURTBOX_COUNTERHIT);
		} else {
			callParams.fillColor = replaceAlpha(alpha, COLOR_HURTBOX);
			callParams.outlineColor = replaceAlpha(255, COLOR_HURTBOX);
		}
		callParams.hatched = state.superArmorActive;
		callParams.originX = params.posX;
		callParams.originY = params.posY;
		*hurtbox = callParams;
		
		if (points && overrideHurtboxType != HITBOXTYPE_HURTBOX && isNotZeroScaled && hurtboxCount) {
			RECT bounds = callParams.getWorldBounds(0);
			DrawPointCallParams pointCallParams;
			pointCallParams.posX = bounds.left;
			pointCallParams.posY = bounds.top;
			points->push_back(pointCallParams);
		}
	}

	bool includeTheseHitboxes = hitboxes && active && !state.doingAThrow;
	if (includeTheseHitboxes) {
		if (ent.dealtAttack()->collisionForceExpand()) {
			includeTheseHitboxes = false;
		}
		if (lastIgnoredHitNum && currentHitNum <= *lastIgnoredHitNum) {
			includeTheseHitboxes = false;
		}
	}
	
	if (!includeTheseHitboxes && hitboxCount && state.doingAThrow && lastIgnoredHitNum) {
		*lastIgnoredHitNum = ent.currentHitNum();
	}

	if (includeTheseHitboxes && isNotZeroScaled && hitboxCount) {

		logOnce(fprintf(logfile, "angle: %d\n", ent.pitch()));
		
		callParams.thickness = THICKNESS_HITBOX;
		
		if (ent.dealtAttack()->clashOnly()) {
			callParams.thickness = 1;
			callParams.fillColor = replaceAlpha(16, COLOR_HITBOX);
		} else {
			callParams.thickness = THICKNESS_HITBOX;
			callParams.fillColor = replaceAlpha(64, COLOR_HITBOX);
		}
		callParams.outlineColor = replaceAlpha(255, COLOR_HITBOX);
		
		if (numHitboxes
				&& !ent.dealtAttack()->clashOnly()  // don't include clash-only hitboxes in the active frames: Venom QV
				&& !ent.isSuperFrozen()  // don't include projectiles stuck in superfreeze as active: Ky CSE YRC
				) {
			*numHitboxes += hitboxCount;
		}
		callParams.hitboxData = hitboxData;
		callParams.hitboxCount = hitboxCount;
		callParams.params = params;
		callParams.params.angle = ent.pitch();
		callParams.params.hitboxOffsetX = ent.hitboxOffsetX();
		callParams.params.hitboxOffsetY = ent.hitboxOffsetY();
		callParams.hatched = false;
		callParams.originX = params.posX;
		callParams.originY = params.posY;
		hitboxes->push_back(callParams);
	}
	
	MoveInfo move;
	if (points && isNotZeroScaled
			&& (
				!state.isASummon
				|| moves.getInfo(move, ownerType, nullptr, ent.animationName(), true)
				&& move.drawProjectileOriginPoint
				|| ownerType == CHARACTER_TYPE_JACKO
				&& ent.servant()
				&& (
					settings.showJackoAegisFieldRange
					&& owner.invulnForAegisField()
					|| settings.showJackoServantAttackRange
				)
			)
	) {
		int* mayBallJumpConnectPtr = nullptr;
		int* mayBallJumpConnectRangePtr = nullptr;
		if (ownerType == CHARACTER_TYPE_JOHNNY) {
			if (circles && strcmp(ent.animationName(), "Mist") == 0) {
				BYTE* func = ent.bbscrCurrentFunc();
				BYTE* instr;
				int radius = 0;
				for (
						instr = moves.skipInstruction(func);
						moves.instructionType(instr) != Moves::instr_endState;
						instr = moves.skipInstruction(instr)
				) {
					if (moves.instructionType(instr) == Moves::instr_ifOperation
							&& *(int*)(instr + 4) == 13  // IS_LESSER_OR_EQUAL
							&& *(int*)(instr + 8) == 2  // tag: variable
							&& *(int*)(instr + 0xc) == 96  // DISTANCE_FROM_THIS_CENTER_TO_ENEMY_CENTER_DUPLICATE
							&& *(int*)(instr + 0x10) == 0) {  // tag: literal
						radius = *(int*)(instr + 0x14);
						break;
					}
				}
				
				if (radius) {
					DrawCircleCallParams newCircle;
					newCircle.posX = state.posX;
					newCircle.posY = state.posY;
					newCircle.radius = radius;
					newCircle.fillColor = D3DCOLOR_ARGB(64, 255, 255, 255);
					newCircle.outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
					circles->push_back(newCircle);
					
					Entity opponent = entityList.slots[1 - ent.team()];
					DrawPointCallParams newPoint;
					newPoint.isProjectile = true;
					newPoint.posX = opponent.posX();
					newPoint.posY = opponent.posY() + opponent.getCenterOffsetY();
					points->push_back(newPoint);
					
					if (lines) {
						DrawLineCallParams newLine;
						newLine.posX1 = newPoint.posX;
						newLine.posY1 = newPoint.posY;
						newLine.posX2 = newCircle.posX;
						newLine.posY2 = newCircle.posY;
						lines->push_back(newLine);
					}
					
				}
			}
		} else if (ownerType == CHARACTER_TYPE_MAY) {
			if (strcmp(ent.animationName(), "MayBallA") == 0) {
				mayBallJumpConnectPtr = &moves.mayPBallJumpConnectOffset;
				mayBallJumpConnectRangePtr = &moves.mayPBallJumpConnectRange;
			} else if (strcmp(ent.animationName(), "MayBallB") == 0) {
				mayBallJumpConnectPtr = &moves.mayKBallJumpConnectOffset;
				mayBallJumpConnectRangePtr = &moves.mayKBallJumpConnectRange;
			}
		}
		if (mayBallJumpConnectPtr) {
			DrawPointCallParams pointCallParams;
			pointCallParams.isProjectile = true;
			pointCallParams.posX = params.posX;
			pointCallParams.posY = params.posY - ent.landingHeight();
			points->push_back(pointCallParams);
			
			if (ent.hasUpon(3) && !settings.dontShowMayInteractionChecks) {
				getMayBallJumpConnectOffsetYAndRange(ent.bbscrCurrentFunc(), mayBallJumpConnectPtr, mayBallJumpConnectRangePtr);
				DrawPointCallParams pointCallParams;
				pointCallParams.isProjectile = true;
				pointCallParams.posX = params.posX;
				pointCallParams.posY = params.posY + *mayBallJumpConnectPtr;
				points->push_back(pointCallParams);
				
				if (*mayBallJumpConnectRangePtr > 0) {
					DrawCircleCallParams circleCallParams;
					circleCallParams.posX = params.posX;
					circleCallParams.posY = params.posY + *mayBallJumpConnectPtr;
					circleCallParams.radius = *mayBallJumpConnectRangePtr;
					circles->push_back(circleCallParams);
				}
			}
		} else {
			DrawPointCallParams pointCallParams;
			pointCallParams.isProjectile = state.isASummon;
			pointCallParams.posX = params.posX;
			pointCallParams.posY = params.posY;
			points->push_back(pointCallParams);
			
			if (ent.characterType() == CHARACTER_TYPE_MAY
					&& !settings.dontShowMayInteractionChecks
					&& !(params.posY == 0 && ent.speedY() == 0)
					&& !ent.inHitstun()) {
				int* mayBallJumpConnectPtr = nullptr;
				int* mayBallJumpConnectRangePtr = nullptr;
				Entity mayBall;
				for (int i = 0; i < entityList.count; ++i) {
					Entity p = entityList.list[i];
					if (p.isActive() && p.team() == state.team && !p.isPawn() && p.hasUpon(3)) {
						if (strcmp(p.animationName(), "MayBallA") == 0) {
							mayBall = p;
							mayBallJumpConnectPtr = &moves.mayPBallJumpConnectOffset;
							mayBallJumpConnectRangePtr = &moves.mayPBallJumpConnectRange;
							break;
						} else if (strcmp(p.animationName(), "MayBallB") == 0) {
							mayBall = p;
							mayBallJumpConnectPtr = &moves.mayKBallJumpConnectOffset;
							mayBallJumpConnectRangePtr = &moves.mayKBallJumpConnectRange;
							break;
						}
					}
				}
				if (mayBallJumpConnectPtr) {
					DrawPointCallParams pointCallParams;
					pointCallParams.isProjectile = true;
					pointCallParams.posX = params.posX;
					pointCallParams.posY = ent.getCenterOffsetY() + params.posY;
					points->push_back(pointCallParams);
					
					getMayBallJumpConnectOffsetYAndRange(mayBall.bbscrCurrentFunc(), mayBallJumpConnectPtr, mayBallJumpConnectRangePtr);
					if (lines) {
						DrawLineCallParams lineCallParams;
						lineCallParams.posX1 = pointCallParams.posX;
						lineCallParams.posY1 = pointCallParams.posY;
						lineCallParams.posX2 = mayBall.x();
						lineCallParams.posY2 = mayBall.y() + *mayBallJumpConnectPtr;
						lines->push_back(lineCallParams);
					}
				}
				
				Entity dolphin = ent.stackEntity(0);
				if (dolphin && dolphin.isActive() && dolphin.mem46()) {
					DrawPointCallParams pointCallParams;
					pointCallParams.isProjectile = true;
					pointCallParams.posX = params.posX + 140000 * (int)params.flip;
					pointCallParams.posY = params.posY + 50000;
					points->push_back(pointCallParams);
					
					DrawCircleCallParams circleCallParams;
					circleCallParams.posX = dolphin.x();
					circleCallParams.posY = dolphin.y();
					circleCallParams.radius = 230000;
					if (circles) {
						circles->push_back(circleCallParams);
					}
					
					if (lines) {
						DrawLineCallParams lineCallParams;
						lineCallParams.posX1 = pointCallParams.posX;
						lineCallParams.posY1 = pointCallParams.posY;
						lineCallParams.posX2 = circleCallParams.posX;
						lineCallParams.posY2 = circleCallParams.posY;
						lines->push_back(lineCallParams);
					}
				}
			} else if (state.charType == CHARACTER_TYPE_MILLIA
				&& lines
				&& settings.showMilliaBadMoonBuffHeight
				&& (
					entityList.slots[1 - state.team].characterType() != CHARACTER_TYPE_MILLIA
					|| state.team == 0
				)) {
				bool isRev2;
				if (moves.milliaIsRev2 == Moves::TRIBOOL_DUNNO) {
					isRev2 = findMoveByName((void*)ent.ent, "SilentForce2", 0) != nullptr;
					if (isRev2) {
						moves.milliaIsRev2 = Moves::TRIBOOL_TRUE;
					} else {
						moves.milliaIsRev2 = Moves::TRIBOOL_FALSE;
					}
				} else {
					isRev2 = moves.milliaIsRev2 == Moves::TRIBOOL_TRUE;
				}
				if (isRev2) {
					DrawLineCallParams lineCallParams;
					lineCallParams.posX1 = -1600000;
					lineCallParams.posY1 = 500000;
					lineCallParams.posX2 = 1600000;
					lineCallParams.posY2 = 500000;
					lines->push_back(lineCallParams);
				}
			} else if (state.charType == CHARACTER_TYPE_BEDMAN
				&& lines
				&& settings.showBedmanTaskCHeightBuffY
				&& (
					entityList.slots[1 - state.team].characterType() != CHARACTER_TYPE_BEDMAN
					|| state.team == 0
				)) {
				DrawLineCallParams lineCallParams;
				lineCallParams.posX1 = -1600000;
				lineCallParams.posY1 = 700000;
				lineCallParams.posX2 = 1600000;
				lineCallParams.posY2 = 700000;
				lines->push_back(lineCallParams);
			}
		}
	}
	
	if (interactionBoxes) {
		if (ownerType == CHARACTER_TYPE_MILLIA) {
			if (ent.mem52() && strcmp(ent.animationName(), "SilentForceKnife") == 0) {
				DrawBoxCallParams interactionBoxParams;
				interactionBoxParams.left = params.posX - 180000;
				interactionBoxParams.right = params.posX + 180000;
				interactionBoxParams.top = params.posY + 10000000;
				interactionBoxParams.bottom = params.posY - 10000000;
				interactionBoxParams.fillColor = replaceAlpha(16, COLOR_INTERACTION);
				interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
				interactionBoxParams.thickness = THICKNESS_INTERACTION;
				interactionBoxes->push_back(interactionBoxParams);
			}
		} else if (ownerType == CHARACTER_TYPE_FAUST) {
			int rangeX = 0;
			int circleRadius = 0;
			if (!ent.mem50() && ent.y() == 0) {
				if (strcmp(ent.animationName(), "Item_Chocolate"_hardcode) == 0) {
					rangeX = 84000;
				} else if (strcmp(ent.animationName(), "Item_BestChocolate"_hardcode) == 0) {
					rangeX = 168000;
				} else if (strcmp(ent.animationName(), "Item_Donut"_hardcode) == 0) {
					rangeX = 84000;
				} else if (strcmp(ent.animationName(), "Item_ManyDonut"_hardcode) == 0) {
					rangeX = 168000;
				}
			}
			if (ent.hasUpon(3) && strcmp(ent.animationName(), "Item_Helium"_hardcode) == 0) {
				circleRadius = 100000;
			}
			if (rangeX) {
				DrawBoxCallParams interactionBoxParams;
				interactionBoxParams.left = params.posX - rangeX;
				interactionBoxParams.right = params.posX + rangeX;
				interactionBoxParams.top = params.posY + 10000000;
				interactionBoxParams.bottom = params.posY - 10000000;
				interactionBoxParams.fillColor = replaceAlpha(16, COLOR_INTERACTION);
				interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
				interactionBoxParams.thickness = THICKNESS_INTERACTION;
				interactionBoxParams.hatched = false;
				interactionBoxParams.originX = params.posX;
				interactionBoxParams.originY = params.posY;
				interactionBoxes->push_back(interactionBoxParams);
			}
			if (circleRadius && circles) {
				DrawCircleCallParams circleCallParams;
				circleCallParams.posX = params.posX;
				circleCallParams.posY = params.posY;
				circleCallParams.radius = circleRadius;
				circles->push_back(circleCallParams);
			}
		} else if (ownerType == CHARACTER_TYPE_KY) {
			int mahojinX = 0;
			int mahojinY = 0;
			int* mahojinCacheVarX = nullptr;
			int* mahojinCacheVarY = nullptr;
			if (ent.mem45()) {
				if (strcmp(ent.animationName(), "StunEdgeObj") == 0) {
					mahojinCacheVarX = &moves.stunEdgeMahojinDistX;
					mahojinCacheVarY = &moves.stunEdgeMahojinDistY;
				} else if (strcmp(ent.animationName(), "ChargedStunEdgeObj") == 0) {
					mahojinCacheVarX = &moves.chargedStunEdgeMahojinDistX;
					mahojinCacheVarY = &moves.chargedStunEdgeMahojinDistY;
				} else if (strcmp(ent.animationName(), "SacredEdgeObj") == 0) {
					mahojinCacheVarX = &moves.sacredEdgeMahojinDistX;
					mahojinCacheVarY = &moves.sacredEdgeMahojinDistY;
				}
			}
			if (mahojinCacheVarX && mahojinCacheVarY) {
				for (int i = 0; i < entityList.count; ++i) {
					Entity it = entityList.list[i];
					if (!it.destructionRequested() && it.team() == ent.team() && !it.isPawn() && strcmp(it.animationName(), "Mahojin") == 0) {
						if (it.mem45()) {
							if (!*mahojinCacheVarX) {
								getMahojinDistXY(ent.bbscrCurrentFunc(), mahojinCacheVarX, mahojinCacheVarY);
							}
							mahojinX = *mahojinCacheVarX;
							mahojinY = *mahojinCacheVarY;
						}
						break;
					}
				}
			}
			if (mahojinX && mahojinY) {
				DrawBoxCallParams interactionBoxParams;
				interactionBoxParams.left = params.posX - mahojinX;
				interactionBoxParams.right = params.posX + mahojinX;
				interactionBoxParams.top = params.posY + mahojinY;
				interactionBoxParams.bottom = params.posY - mahojinY;
				interactionBoxParams.fillColor = replaceAlpha(16, COLOR_INTERACTION);
				interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
				interactionBoxParams.thickness = THICKNESS_INTERACTION;
				interactionBoxes->push_back(interactionBoxParams);
			}
		} else if (ownerType == CHARACTER_TYPE_JACKO) {
			if (ent.ghost() && ent.y() == 0 && ent.displayModel() && settings.showJackoGhostPickupRange) {
				if (moves.ghostPickupRange == 0) {
					BYTE* func = owner.findSubroutineStart("OnFrameStep");
					if (func) {
						BYTE* instr;
						for (
								instr = moves.skipInstruction(func);
								moves.instructionType(instr) != Moves::instr_endState;
								instr = moves.skipInstruction(instr)
						) {
							if (moves.instructionType(instr) == Moves::instr_ifOperation
									&& *(int*)(instr + 4) == 11  // IS_LESSER
									&& *(int*)(instr + 8) == 2  // tag:variable
									&& *(int*)(instr + 0xc) == 53  // Mem(53)
									&& *(int*)(instr + 0x10) == 0) {  // tag:literal
								moves.ghostPickupRange = *(int*)(instr + 0x14);
								break;
							}
						}
					}
				}
				DrawBoxCallParams interactionBoxParams;
				interactionBoxParams.left = params.posX - moves.ghostPickupRange;
				interactionBoxParams.right = params.posX + moves.ghostPickupRange;
				interactionBoxParams.top = params.posY + 10000000;
				interactionBoxParams.bottom = params.posY - 1000000;
				interactionBoxParams.fillColor = replaceAlpha(16, COLOR_INTERACTION);
				interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
				interactionBoxParams.thickness = THICKNESS_INTERACTION;
				interactionBoxes->push_back(interactionBoxParams);
			} else if (ent.servant() && ent.displayModel() && settings.showJackoServantAttackRange) {
				int mem45 = ent.mem45();
				if (mem45 != 0  // spawn
						&& mem45 != 4) {  // death of various kinds
					int* aggroX = nullptr;
					int* aggroY = nullptr;
					if (ent.servantA()) {
						aggroX = &moves.jackoServantAAggroX;
						aggroY = &moves.jackoServantAAggroY;
					} else if (ent.servantB()) {
						aggroX = &moves.jackoServantBAggroX;
						aggroY = &moves.jackoServantBAggroY;
					} else if (ent.servantC()) {
						aggroX = &moves.jackoServantCAggroX;
						aggroY = &moves.jackoServantCAggroY;
					}
					
					if (*aggroX == 0) {
						BYTE* func = ent.bbscrCurrentFunc();
						BYTE* instr;
						for (
								instr = moves.skipInstruction(func);
								moves.instructionType(instr) != Moves::instr_endState;
								instr = moves.skipInstruction(instr)
						) {
							if (moves.instructionType(instr) == Moves::instr_ifOperation
									&& *(int*)(instr + 4) == 12  // IS_GREATER_OR_EQUAL
									&& *(int*)(instr + 8) == 2  // tag:variable
									&& *(int*)(instr + 0xc) == 104  // OPPONENT_X_OFFSET_TOWARDS_FACING
									&& *(int*)(instr + 0x10) == 0  // tag:literal
									&& *(int*)(instr + 0x14) == 0  // 0
									&& moves.instructionType(instr + 0x18) == Moves::instr_ifOperation
									&& *(int*)(instr + 0x1c) == 13  // IS_LESSER_OR_EQUAL
									&& *(int*)(instr + 0x20) == 2  // tag:variable
									&& *(int*)(instr + 0x24) == 104  // OPPONENT_X_OFFSET_TOWARDS_FACING
									&& *(int*)(instr + 0x28) == 0  // tag:literal
									// skip the value of the literal - we'll read it later
									&& moves.instructionType(instr + 0x30) == Moves::instr_ifOperation
									&& *(int*)(instr + 0x34) == 13  // IS_LESSER_OR_EQUAL
									&& *(int*)(instr + 0x38) == 2  // tag:variable
									&& *(int*)(instr + 0x3c) == 15  // OPPONENT_Y_DISTANCE
									&& *(int*)(instr + 0x40) == 0) {  // tag:literal
								*aggroX = *(int*)(instr + 0x2c);
								*aggroY = *(int*)(instr + 0x44);
								break;
							}
						}
					}
					
					DrawBoxCallParams interactionBoxParams;
					interactionBoxParams.left = params.posX - *aggroX;
					interactionBoxParams.right = params.posX + *aggroX;
					interactionBoxParams.top = params.posY + *aggroY;
					interactionBoxParams.bottom = params.posY - *aggroY;
					interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
					interactionBoxParams.thickness = THICKNESS_INTERACTION;
					interactionBoxes->push_back(interactionBoxParams);
				}
			}
		} else if (state.charType == CHARACTER_TYPE_JAM) {
			if (strcmp(ent.animationName(), "Saishingeki") == 0
					&& ent.currentHitNum() == 2
					&& hitboxCount) {
				moves.fillInJamSaishingekiY(ent.bbscrCurrentFunc());
				
				DrawBoxCallParams interactionBoxParams;
				interactionBoxParams.left = params.posX - 10000000;
				interactionBoxParams.right = params.posX + 10000000;
				interactionBoxParams.top = params.posY + moves.jamSaishingekiY;
				interactionBoxParams.bottom = params.posY;
				interactionBoxParams.fillColor = replaceAlpha(64, COLOR_INTERACTION);
				interactionBoxParams.outlineColor = replaceAlpha(255, COLOR_INTERACTION);
				interactionBoxParams.thickness = THICKNESS_INTERACTION;
				interactionBoxes->push_back(interactionBoxParams);
			}
		}
	}
	if (circles) {
		if (state.charType == CHARACTER_TYPE_FAUST) {
			bool needShow = false;
			bool needFill = false;
			if (settings.showFaustOwnFlickRanges && strcmp(ent.animationName(), "NmlAtk5E") == 0) {
				if (strcmp(ent.spriteName(), "fau205_05") == 0) {
					needShow = true;
				} else if (strcmp(ent.spriteName(), "fau205_06") == 0) {
					needShow = true;
					needFill = ent.spriteFrameCounter() == 0 && !ent.isRCFrozen();
					if (moves.faust5DExPointX == -1) {
						HitboxType hitboxType = HITBOXTYPE_EX_POINT;
						int count = ent.hitboxCount(HITBOXTYPE_EX_POINT);
						if (count == 0) {
							hitboxType = HITBOXTYPE_EX_POINT_EXTENDED;
							count = ent.hitboxCount(HITBOXTYPE_EX_POINT_EXTENDED);
						}
						if (count) {
							DrawHitboxArrayCallParams dummyParams;
							dummyParams.hitboxData = ent.hitboxData(hitboxType);
							dummyParams.hitboxCount = 1;
							dummyParams.params = params;
							
							RECT boxBounds = dummyParams.getWorldBounds(0);
							moves.faust5DExPointX = (boxBounds.left - params.posX) * (int)params.flip;
							moves.faust5DExPointY = boxBounds.top - params.posY;
						}
					}
				} else if (strcmp(ent.spriteName(), "fau205_07") == 0
						|| strcmp(ent.spriteName(), "fau205_08") == 0 && ent.spriteFrameCounter() < 6) {
					needShow = true;
				}
			}
			if (needShow && moves.faust5DExPointX != -1) {
				
				DrawCircleCallParams circleCallParams;
				circleCallParams.posX = params.posX + moves.faust5DExPointX * (int)params.flip;
				circleCallParams.posY = params.posY + moves.faust5DExPointY;
				circleCallParams.radius = 100000;
				if (needFill) {
					circleCallParams.fillColor = D3DCOLOR_ARGB(64, 255, 255, 255);
				}
				circles->push_back(circleCallParams);
				
				circleCallParams.radius = 300000;
				circles->push_back(circleCallParams);
				
				if (points) {
					DrawPointCallParams pointCallParams;
					pointCallParams.isProjectile = true;
					pointCallParams.posX = circleCallParams.posX;
					pointCallParams.posY = circleCallParams.posY;
					points->push_back(pointCallParams);
				}
			}
		} else if (ownerType == CHARACTER_TYPE_JACKO) {
			if (strcmp(ent.animationName(), "Aigisfield") == 0
					&& settings.showJackoAegisFieldRange) {
				if (moves.jackoAegisFieldRange == 0) {
					BYTE* func = ent.findStateStart("ServantA");
					if (func) {
						BYTE* instr;
						for (
								instr = moves.skipInstruction(func);
								moves.instructionType(instr) != Moves::instr_endState;
								instr = moves.skipInstruction(instr)
						) {
							if (moves.instructionType(instr) == Moves::instr_calcDistance
									&& *(int*)(instr + 4) == 3  // PLAYER
									&& *(int*)(instr + 8) == 103  // CENTER
									&& *(int*)(instr + 0xc) == 23  // SELF
									&& *(int*)(instr + 0x10) == 103  // CENTER
									&& moves.instructionType(instr + 0x14) == Moves::instr_ifOperation
									&& *(int*)(instr + 0x18) == 11  // IS_LESSER
									&& *(int*)(instr + 0x1c) == 2  // tag:variable
									&& *(int*)(instr + 0x20) == 0  // Mem(ACCUMULATOR)
									&& *(int*)(instr + 0x24) == 0) {  // tag:literal
								moves.jackoAegisFieldRange = *(int*)(instr + 0x28);
								break;
							}
						}
					}
				}
				if (moves.jackoAegisFieldRange) {
					int centerX = owner.posX();
					int centerY = owner.posY() + owner.getCenterOffsetY();
					if (points) {
						DrawPointCallParams pointCallParams;
						pointCallParams.isProjectile = true;
						pointCallParams.posX = centerX;
						pointCallParams.posY = centerY;
						points->push_back(pointCallParams);
					}
					
					
					DrawCircleCallParams circleCallParams;
					circleCallParams.posX = centerX;
					circleCallParams.posY = centerY;
					circleCallParams.radius = moves.jackoAegisFieldRange;
					circleCallParams.outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
					circles->push_back(circleCallParams);
					
				}
			}
		} else if (state.charType == CHARACTER_TYPE_HAEHYUN) {
			if (strcmp(ent.animationName(), "BlackHoleAttack") == 0 && ent.mem45() && ent.hasUpon(3)) {
				int centerX = ent.posX();
				int centerY = ent.posY() + ent.getCenterOffsetY();
				Entity opponent = ent.enemyEntity();
				int centerX2 = opponent.posX();
				int centerY2 = opponent.posY() + opponent.getCenterOffsetY();
				if (points) {
					DrawPointCallParams pointCallParams;
					pointCallParams.isProjectile = true;
					pointCallParams.posX = centerX;
					pointCallParams.posY = centerY;
					points->push_back(pointCallParams);
					
					pointCallParams.isProjectile = true;
					pointCallParams.posX = centerX2;
					pointCallParams.posY = centerY2;
					points->push_back(pointCallParams);
				}
				if (lines) {
					DrawLineCallParams lineCallParams;
					lineCallParams.posX1 = centerX;
					lineCallParams.posY1 = centerY;
					lineCallParams.posX2 = centerX2;
					lineCallParams.posY2 = centerY2;
					lines->push_back(lineCallParams);
				}
				
				DrawCircleCallParams circleCallParams;
				circleCallParams.posX = centerX;
				circleCallParams.posY = centerY;
				circleCallParams.radius = 1100000;
				circleCallParams.outlineColor = D3DCOLOR_ARGB(255, 255, 255, 255);
				circles->push_back(circleCallParams);
				
			}
		}
	}
	
}

void getMayBallJumpConnectOffsetYAndRange(BYTE* functionStart, int* mayBallJumpConnectPtr, int* mayBallJumpConnectRangePtr) {
	if (*mayBallJumpConnectPtr == 0) {
		bool foundY = false;
		bool foundRange = false;
		for (
				BYTE* instr = moves.skipInstruction(functionStart);
				moves.instructionType(instr) != Moves::instr_endState;
				instr = moves.skipInstruction(instr)
		) {
			if (!foundY) {
				if (moves.instructionType(instr) == Moves::instr_exPointFReset
						&& *(int*)(instr + 4) == 100) {  // ORIGIN
					*mayBallJumpConnectPtr = *(int*)(instr + 0xc);
					foundY = true;
					if (foundRange) break;
				}
			}
			if (!foundRange) {
				if (moves.instructionType(instr) == Moves::instr_ifOperation
						&& *(int*)(instr + 4) == 11  // IS_LESSER
						&& *(int*)(instr + 8) == 2  // tag: variable
						&& *(int*)(instr + 0xc) == 0) {  // Mem(ACCUMULATOR)
					// skip tag: literal
					*mayBallJumpConnectRangePtr = *(int*)(instr + 0x14);
					foundRange = true;
					if (foundY) break;
				}
			}
		}
	}
}

void getMahojinDistXY(BYTE* functionStart, int* x, int* y) {
	for (BYTE* instr = functionStart; moves.instructionType(instr) != Moves::instr_endState; instr = moves.skipInstruction(instr)) {
		if (moves.instructionType(instr) == Moves::instr_ifOperation
				&& *(int*)(instr + 0x4) == 11    // IS_LESSER
				&& *(int*)(instr + 0x8) == 2) {  // tag: variable
			if (*(int*)(instr + 0xc) == 62  // GTMP_Y
					&& *(int*)(instr + 0x10) == 0  // tag: value
					&& *(int*)(instr + 0x14) != 0) {
				*x = *(int*)(instr + 0x14);
			} else if (*(int*)(instr + 0xc) == 64  // MEM(64)
					&& *(int*)(instr + 0x10) == 0  // tag: value
					&& *(int*)(instr + 0x14) != 0) {
				*y = *(int*)(instr + 0x14);
				return;
			}
		}
	}
}
