#pragma once
#include "Entity.h"
#include <vector>
enum InputNameType {
	BUTTON,
	MULTIWORD_BUTTON,
	MOTION,
	MULTIWORD_MOTION
};
struct InputName {
	const char* name;
	InputNameType type;
	int windowDuration;  // includes the first actionable frame, or the frame on which the thing that you're buffering would get performed
};
void fillInInputNames();
extern std::vector<InputName> inputNames;
