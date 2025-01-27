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
	int bufferTime;
};
void fillInInputNames();
extern std::vector<InputName> inputNames;
