#pragma once
#include <d3d9.h>
#include <vector>

// Everything here is not thread-safe

struct PathElement {
	int x = 0;
	int y = 0;
	int inX = 0;  // direction in which the filled region is from this corner. May only be 1 or -1
	int inY = 0;
	PathElement(int x, int y, int inX, int inY);
};

class DrawOutlineCallParams {
public:
	D3DCOLOR outlineColor{ 0 };
	int thickness = 0;

	// You must reserve size first for N elems and only then add them
	void reserveSize(int numPathElems);
	void addPathElem(int x, int y, int inX, int inY);
	const PathElement& getPathElem(int index) const;
	int count() const;
private:
	int outlineStartAddr = 0;
	int outlineCount = 0;
	int internalOutlineAddr = -1;
};

class DrawOutlineCallParamsManager {
public:
	void onEndSceneStart();
private:
	friend class DrawOutlineCallParams;
	std::vector<PathElement> allPathElems;
};

extern DrawOutlineCallParamsManager drawOutlineCallParamsManager;
