#pragma once

struct BoundingRect {
	float left = 0;
	float right = 0;
	float top = 0;
	float bottom = 0;
	bool emptyX = true;
	bool emptyY = true;
	void addX(float x);
	void addY(float y);
};
