#include "pch.h"
#include "collectHitboxes.h"
#include "logging.h"
#include "pi.h"
#include "EntityList.h"
#include "Hitbox.h"
#include "colors.h"

const int pushboxThickness = 1;
const int boxThickness = 3000;
const int wayTooNumerousSummonsThickness = 1;

void collectHitboxes(const Entity& ent,
		const bool active,
		DrawHitboxArrayCallParams* hurtbox,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
		std::vector<DrawBoxCallParams>* const pushboxes) {

	EntityState state;
	ent.getState(&state);

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

	if (pushboxes && !state.isASummon) {
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
		pushboxParams.thickness = pushboxThickness;
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
	callParams.thickness = boxThickness;

	if (hurtbox) {
		callParams.hitboxData = hurtboxData;
		callParams.hitboxCount = hurtboxCount;
		callParams.params = params;
		DWORD alpha = 64;
		if (state.strikeInvuln) {
			alpha = 0;
		} else if (state.isASummon && state.ownerCharType == CHARACTER_TYPE_KY) {
			alpha = 0;
		} else if (state.isASummon && (state.ownerCharType == CHARACTER_TYPE_JACKO || state.ownerCharType == CHARACTER_TYPE_BEDMAN)) {
			alpha = 32;
			callParams.thickness = wayTooNumerousSummonsThickness;
		}
		if (state.counterhit) {
			callParams.fillColor = replaceAlpha(alpha, COLOR_HURTBOX_COUNTERHIT);
			callParams.outlineColor = replaceAlpha(255, COLOR_HURTBOX_COUNTERHIT);
		} else {
			callParams.fillColor = replaceAlpha(alpha, COLOR_HURTBOX);
			callParams.outlineColor = replaceAlpha(255, COLOR_HURTBOX);
		}
		*hurtbox = callParams;
	}

	if (hitboxes && active && !state.doingAThrow) {
		logOnce(fprintf(logfile, "angle: %d\n", *(int*)(ent + 0x258)));
		
		if (state.isASummon && state.ownerCharType == CHARACTER_TYPE_JACKO && hitboxCount == 1 && hitboxData->sizeX == 449.F && hitboxData->sizeY == 416.F) {
			callParams.thickness = 1;
			callParams.fillColor = replaceAlpha(16, COLOR_HITBOX);
		} else {
			callParams.thickness = boxThickness;
			callParams.fillColor = replaceAlpha(64, COLOR_HITBOX);
		}
		callParams.outlineColor = replaceAlpha(255, COLOR_HITBOX);
		
		callParams.hitboxData = hitboxData;
		callParams.hitboxCount = hitboxCount;
		callParams.params = params;
		callParams.params.angle = *(int*)(ent + 0x258);
		callParams.params.hitboxOffsetX = *(int*)(ent + 0x27C);
		callParams.params.hitboxOffsetY = *(int*)(ent + 0x280);
		hitboxes->push_back(callParams);
	}

	if (points && !state.isASummon) {
		DrawPointCallParams pointCallParams;
		pointCallParams.posX = params.posX;
		pointCallParams.posY = params.posY;
		points->push_back(pointCallParams);
	}
}
