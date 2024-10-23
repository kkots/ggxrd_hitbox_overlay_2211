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
		int* numHitboxes,
		int* lastIgnoredHitNum) {
	
	if (!ent.isPawn()
			&& (ent.team() == 0 || ent.team() == 1)
			&& entityList.slots[ent.team()].characterType() == CHARACTER_TYPE_JACKO
			&& !ent.displayModel()
			|| ent.y() < -3000000  // needed for May [2]8S/H
			|| ent.isHidden()) {  // needed for super animations
		return;
	}
	
	EntityState state;
	ent.getState(&state);
	
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
		pushboxes->push_back(pushboxParams);
		logOnce(fputs("pushboxes->push_back(...) called\n", logfile));
		//}
	}

	const Hitbox* const hurtboxData = *(const Hitbox* const*)(ent + 0x58);
	const Hitbox* const hitboxData = *(const Hitbox* const*)(ent + 0x5C);

	const int hurtboxCount = *(const int*)(ent + 0xA0);
	const int hitboxCount = *(const int*)(ent + 0xA4);

	logOnce(fprintf(logfile, "hurtbox_count: %d; hitbox_count: %d\n", hurtboxCount, hitboxCount));

	// Thanks to jedpossum on dustloop for these offsets
	params.scaleX = *(int*)(ent + 0x264);
	params.scaleY = *(int*)(ent + 0x268);
	logOnce(fprintf(logfile, "scale_x: %d; scale_y: %d\n", *(int*)(ent + 0x264), *(int*)(ent + 0x268)));

	DrawHitboxArrayCallParams callParams;

	if (hurtbox && isNotZeroScaled) {
		callParams.thickness = THICKNESS_HURTBOX;
		callParams.hitboxData = hurtboxData;
		callParams.hitboxCount = hurtboxCount;
		callParams.params = params;
		callParams.params.angle = *(int*)(ent + 0x258);
		callParams.params.hitboxOffsetX = *(int*)(ent + 0x27C);
		callParams.params.hitboxOffsetY = *(int*)(ent + 0x280);
		DWORD alpha = 64;
		if (state.strikeInvuln) {
			alpha = 0;
		} else if (state.isASummon && state.ownerCharType == CHARACTER_TYPE_KY) {
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
		*hurtbox = callParams;
	}

	bool includeTheseHitboxes = hitboxes && active && !(state.doingAThrow && ent.isPawn());  // isPawn check for Dizzy bubble pop
	if (includeTheseHitboxes) {
		if ((*(DWORD*)(ent + 0x44c + 0x14) & 0x4) != 0) {  // having this flag means you ignore the hitboxes hit detection check
			includeTheseHitboxes = false;
		}
		if (lastIgnoredHitNum && currentHitNum <= *lastIgnoredHitNum) {
			includeTheseHitboxes = false;
		}
	}
	
	if (!includeTheseHitboxes && hitboxCount && state.doingAThrow && lastIgnoredHitNum) {
		*lastIgnoredHitNum = ent.currentHitNum();
	}

	if (includeTheseHitboxes && isNotZeroScaled) {

		logOnce(fprintf(logfile, "angle: %d\n", *(int*)(ent + 0x258)));
		
		callParams.thickness = THICKNESS_HITBOX;
		
		if (ent.clashOnly()) {
			callParams.thickness = 1;
			callParams.fillColor = replaceAlpha(16, COLOR_HITBOX);
		} else {
			callParams.thickness = THICKNESS_HITBOX;
			callParams.fillColor = replaceAlpha(64, COLOR_HITBOX);
		}
		callParams.outlineColor = replaceAlpha(255, COLOR_HITBOX);
		
		if (numHitboxes && !ent.clashOnly()) *numHitboxes += hitboxCount;  // don't include clash-only hitboxes in the active frames: Venom QV
		callParams.hitboxData = hitboxData;
		callParams.hitboxCount = hitboxCount;
		callParams.params = params;
		callParams.params.angle = *(int*)(ent + 0x258);
		callParams.params.hitboxOffsetX = *(int*)(ent + 0x27C);
		callParams.params.hitboxOffsetY = *(int*)(ent + 0x280);
		hitboxes->push_back(callParams);
	}

	if (points && !state.isASummon && isNotZeroScaled) {
		DrawPointCallParams pointCallParams;
		pointCallParams.posX = params.posX;
		pointCallParams.posY = params.posY;
		points->push_back(pointCallParams);
	}
}
