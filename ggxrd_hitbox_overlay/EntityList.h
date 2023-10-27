#pragma once

class EntityList
{
public:
	void onEndSceneStart();
	int count = 0;
	const char** list = nullptr;
	const char** slots = nullptr;
};

extern EntityList entityList;
