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
		bool* wasFullInvul) {
	
	CharacterType ownerType = (CharacterType)-1;
	if (!ent.isPawn()
			&& (ent.team() == 0 || ent.team() == 1)) {
		ownerType = entityList.slots[ent.team()].characterType();
	}
	if (ownerType == CHARACTER_TYPE_JACKO
			&& !ent.displayModel()
			|| ent.y() < -3000000  // needed for May [2]8S/H
			|| ent.isHidden()  // needed for super animations
			|| ownerType == CHARACTER_TYPE_LEO
			&& strcmp(ent.animationName(), "Semuke5E_Reflect"_hardcode) == 0) {
		return;
	}
	
	EntityState state;
	ent.getState(&state, wasSuperArmorEnabled, wasFullInvul);
	if (entityState) *entityState = state;
	
	int currentHitNum = ent.currentHitNum();
	
	DrawHitboxArrayParams params;
	params.posX = ent.posX();
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

	bool isNotZeroScaled = *(int*)(ent + 0x2594) != 0;

	if (pushboxes && !state.isASummon && isNotZeroScaled) {
		logOnce(fputs("Need pushbox\n", logfile));
		// Draw pushbox and throw box
		/*if (is_push_active(asw_data))
		{*/
		const auto pushboxTop = ent.pushboxTop();
		const auto pushboxBottom = ent.pushboxBottom();

		DrawBoxCallParams pushboxParams;
		ent.pushboxLeftRight(&pushboxParams.left, &pushboxParams.right);
		pushboxParams.top = params.posY + pushboxTop;
		pushboxParams.bottom = params.posY - pushboxBottom;
		pushboxParams.fillColor = replaceAlpha(state.throwInvuln ? 0 : 64, COLOR_PUSHBOX);
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
	params.scaleX = ent.scaleX();
	params.scaleY = ent.scaleY();
	logOnce(fprintf(logfile, "scale_x: %d; scale_y: %d\n", *(int*)(ent + 0x264), *(int*)(ent + 0x268)));
	
	DrawHitboxArrayCallParams callParams;
	
	if (hurtbox && isNotZeroScaled && hurtboxCount) {
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
		} else if (THICKNESS_WAY_TOO_NUMEROUS_SUMMONS
				&& state.isASummon && (state.ownerCharType == CHARACTER_TYPE_JACKO || state.ownerCharType == CHARACTER_TYPE_BEDMAN)) {
			alpha = 32;
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
			)
	) {
		int* mayBallJumpConnectPtr = nullptr;
		int* mayBallJumpConnectRangePtr = nullptr;
		if (ownerType == CHARACTER_TYPE_MAY) {
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
			if (!ent.mem50() && ent.y() == 0) {
				int rangeX = 0;
				if (strcmp(ent.animationName(), "Item_Chocolate"_hardcode) == 0) {
					rangeX = 84000;
				} else if (strcmp(ent.animationName(), "Item_BestChocolate"_hardcode) == 0) {
					rangeX = 168000;
				} else if (strcmp(ent.animationName(), "Item_Donut"_hardcode) == 0) {
					rangeX = 84000;
				} else if (strcmp(ent.animationName(), "Item_ManyDonut"_hardcode) == 0) {
					rangeX = 168000;
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
