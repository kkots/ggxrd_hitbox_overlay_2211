#include "pch.h"
#include "BoundingRect.h"

void BoundingRect::addX(float x) {
	if (emptyX || x < left) left = x;
	if (emptyX || x > right) right = x;
	emptyX = false;
}

void BoundingRect::addY(float y) {
	if (emptyY || y < top) top = y;
	if (emptyY || y > bottom) bottom = y;
	emptyY = false;
}
