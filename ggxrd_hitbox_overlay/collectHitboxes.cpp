#include "pch.h"
#include "collectHitboxes.h"
#include "logging.h"
#include "pi.h"
#include "EntityList.h"
#include "Hitbox.h"
#include "colors.h"

void collectHitboxes(Entity ent,
		const bool active,
		DrawHitboxArrayCallParams* const hurtbox,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
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
			&& strcmp(ent.animationName(), "Semuke5E_Reflect") == 0) {
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
		if (ent.collisionForceExpand()) {
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
		
		if (ent.clashOnly()) {
			callParams.thickness = 1;
			callParams.fillColor = replaceAlpha(16, COLOR_HITBOX);
		} else {
			callParams.thickness = THICKNESS_HITBOX;
			callParams.fillColor = replaceAlpha(64, COLOR_HITBOX);
		}
		callParams.outlineColor = replaceAlpha(255, COLOR_HITBOX);
		
		if (numHitboxes
				&& !ent.clashOnly()  // don't include clash-only hitboxes in the active frames: Venom QV
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
	
	if (points && !state.isASummon && isNotZeroScaled) {
		DrawPointCallParams pointCallParams;
		pointCallParams.posX = params.posX;
		pointCallParams.posY = params.posY;
		points->push_back(pointCallParams);
	}
	
	if (interactionBoxes) {
		if (ownerType == CHARACTER_TYPE_FAUST && !ent.mem50() && ent.y() == 0) {
			int rangeX = 0;
			if (strcmp(ent.animationName(), "Item_Chocolate") == 0) {
				rangeX = 84000;
			} else if (strcmp(ent.animationName(), "Item_BestChocolate") == 0) {
				rangeX = 168000;
			} else if (strcmp(ent.animationName(), "Item_Donut") == 0) {
				rangeX = 84000;
			} else if (strcmp(ent.animationName(), "Item_ManyDonut") == 0) {
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
	}
	
}
