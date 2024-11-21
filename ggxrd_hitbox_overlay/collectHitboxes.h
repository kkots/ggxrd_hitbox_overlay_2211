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
		std::vector<DrawBoxCallParams>* const pushboxes,
		int* numHitboxes = nullptr,
		int* lastIgnoredHitNum = nullptr,
		EntityState* entityState = nullptr);
