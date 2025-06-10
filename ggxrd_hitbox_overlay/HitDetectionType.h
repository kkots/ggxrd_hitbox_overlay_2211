#pragma once

enum HitDetectionType {
	HIT_DETECTION_EASY_CLASH,  // only player attacks that have easyClash on
	HIT_DETECTION_NORMAL,  // the regular attack collisions
	HIT_DETECTION_CLASH  // clashes between hitboxes
};
