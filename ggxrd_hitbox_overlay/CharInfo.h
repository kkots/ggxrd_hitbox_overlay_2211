#pragma once

struct CharInfo {
	float uStart;
	float vStart;
	float uEnd;
	float vEnd;
	int sizeX;
	int sizeY;
	int offsetY;  // I eyed the values for these by looking at the Damage output in Training Mode at 1280x720
	int extraSpaceRight;  // I eyed the values for these by looking at the Damage output in Training Mode at 1280x720
};
