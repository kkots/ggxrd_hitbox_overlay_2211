#pragma once
#include <vector>
#include "Entity.h"
#include "DrawHitboxArrayCallParams.h"
#include "DrawPointCallParams.h"
#include "DrawBoxCallParams.h"

void collectHitboxes(const Entity& ent,
		const bool active,
		DrawHitboxArrayCallParams* hurtbox,
		std::vector<DrawHitboxArrayCallParams>* const hitboxes,
		std::vector<DrawPointCallParams>* const points,
		std::vector<DrawBoxCallParams>* const pushboxes);
