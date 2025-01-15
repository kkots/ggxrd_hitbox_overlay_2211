#pragma once
#include <vector>
#include "Entity.h"
#include "DrawHitboxArrayCallParams.h"
#include "DrawPointCallParams.h"
#include "DrawBoxCallParams.h"

void collectHitboxes(Entity ent,
		const bool active,
		DrawHitboxArrayCallParams* const hurtbox,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
		std::vector<DrawLineCallParams>* const lines,
		std::vector<DrawCircleCallParams>* const circles,
		std::vector<DrawBoxCallParams>* const pushboxes,
		std::vector<DrawBoxCallParams>* const interactionBoxes,
		int* numHitboxes = nullptr,
		int* lastIgnoredHitNum = nullptr,
		EntityState* entityState = nullptr,
		bool* wasSuperArmorEnabled = nullptr,
		bool* wasFullInvul = nullptr,
		int scaleX = INT_MAX,
		int scaleY = INT_MAX);
