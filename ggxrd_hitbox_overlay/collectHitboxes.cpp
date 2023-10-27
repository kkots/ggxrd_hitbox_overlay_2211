#include "pch.h"
#include "collectHitboxes.h"
#include "logging.h"
#include "pi.h"

const int pushboxThickness = 1;

void collectHitboxes(const Entity& ent,
		const bool active,
		std::vector<DrawHitboxArrayCallParams>* const hurtboxes,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
		std::vector<DrawBoxCallParams>* const pushboxes) {
	DrawHitboxArrayParams params;
	params.posX = ent.posX();
	params.posY = ent.posY();
	logOnce(fprintf(logfile, "pox_x: %d\n", params.posX));
	logOnce(fprintf(logfile, "pox_y: %d\n", params.posY));
	params.flip = ent.isFacingLeft() ? 1 : -1;
	logOnce(fprintf(logfile, "flip: %d\n", (int)params.flip));

	const bool doingAThrow = ent.isDoingAThrow();
	logOnce(fprintf(logfile, "doingAThrow: %d\n", (int)doingAThrow));
	const bool isGettingThrown = ent.isGettingThrown();
	logOnce(fprintf(logfile, "isGettingThrown: %d\n", (int)isGettingThrown));

	const auto otg = false;//(*(int*)(asw_data + 0x2410) & 0x800000) != 0;  // not found yet
	const auto invulnFrames = *(int*)(ent + 0x9A0);
	logOnce(fprintf(logfile, "invulnFrames: %x\n", invulnFrames));
	const auto invulnFlags = *(char*)(ent + 0x238);
	logOnce(fprintf(logfile, "invulnFlags: %x\n", invulnFlags));
	const auto strikeInvuln = invulnFrames > 0 || (invulnFlags & 16) || (invulnFlags & 64) || doingAThrow || isGettingThrown;
	logOnce(fprintf(logfile, "strikeInvuln: %u\n", (int)strikeInvuln));
	const auto throwInvuln = (invulnFlags & 32) || (invulnFlags & 64) || otg;
	logOnce(fprintf(logfile, "throwInvuln: %u\n", (int)throwInvuln));
	bool isASummon = ent.characterType() == -1;
	logOnce(fprintf(logfile, "isASummon: %u\n", (int)isASummon));
	const auto counterhit = (*(int*)(ent + 0x234) & 256) != 0  // Thanks to WorseThanYou for finding this
		&& !strikeInvuln
		&& !isASummon;
	logOnce(fprintf(logfile, "counterhit: %u\n", (int)counterhit));

	// Color scheme:
	// light blue - hurtbox on counterhit
	// green - hurtbox on not counterhit
	// red - hitbox
	// yellow - pushbox
	// blue - throwbox

	if (pushboxes && !isASummon) {
		logOnce(fputs("Need pushbox\n", logfile));
		// Draw pushbox and throw box
		/*if (is_push_active(asw_data))
		{*/
		const auto pushboxTop = ent.pushboxTop();
		logOnce(fprintf(logfile, "pushboxTop: %d\n", pushboxTop));
		const auto pushboxBottom = ent.pushboxBottom();
		logOnce(fprintf(logfile, "pushboxBottom: %d\n", pushboxBottom));

		const auto pushboxWidth = ent.pushboxWidth();
		logOnce(fprintf(logfile, "pushboxWidth: %d\n", pushboxWidth));
		const auto pushboxBack = pushboxWidth / 2;
		logOnce(fprintf(logfile, "pushboxBack: %d\n", pushboxBack));
		const auto frontOffset = ent.pushboxFrontWidthOffset();
		logOnce(fprintf(logfile, "frontOffset: %d\n", frontOffset));
		const auto pushboxFront = pushboxWidth / 2 + frontOffset;
		logOnce(fprintf(logfile, "pushboxFront: %d\n", pushboxFront));

		DrawBoxCallParams pushboxParams;
		pushboxParams.left = params.posX - pushboxBack * params.flip;
		pushboxParams.right = params.posX + pushboxFront * params.flip;
		pushboxParams.top = params.posY + pushboxTop;
		pushboxParams.bottom = params.posY - pushboxBottom;
		pushboxParams.fillColor = D3DCOLOR_ARGB(throwInvuln ? 0 : 64, 255, 255, 0);
		pushboxParams.outlineColor = D3DCOLOR_ARGB(255, 255, 255, 0);
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

	if (hurtboxes) {
		callParams.hitboxData = hurtboxData;
		callParams.hitboxCount = hurtboxCount;
		callParams.params = params;
		callParams.fillColor = D3DCOLOR_ARGB(strikeInvuln ? 0 : 64, 0, 255, counterhit ? 255 : 0);
		callParams.outlineColor = D3DCOLOR_ARGB(255, 0, 255, counterhit ? 255 : 0);
		hurtboxes->push_back(callParams);
	}

	if (hitboxes && active && !doingAThrow) {
		logOnce(fprintf(logfile, "angle: %d\n", *(int*)(ent + 0x258)));

		callParams.hitboxData = hitboxData;
		callParams.hitboxCount = hitboxCount;
		callParams.params = params;
		callParams.params.angle = *(int*)(ent + 0x258);
		callParams.params.hitboxOffsetX = *(int*)(ent + 0x27C);
		callParams.params.hitboxOffsetY = *(int*)(ent + 0x280);
		callParams.fillColor = D3DCOLOR_ARGB(64, 255, 0, 0);
		callParams.outlineColor = D3DCOLOR_ARGB(255, 255, 0, 0);
		hitboxes->push_back(callParams);
	}

	if (points) {
		DrawPointCallParams pointCallParams;
		pointCallParams.posX = params.posX;
		pointCallParams.posY = params.posY;
		points->push_back(pointCallParams);
	}
}
