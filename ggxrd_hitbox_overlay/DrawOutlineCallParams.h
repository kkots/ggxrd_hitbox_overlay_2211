#pragma once
#include <d3d9.h>
#include <vector>

// Everything here is not thread-safe

struct PathElement {
	int x = 0;
	int y = 0;
	bool hasProjectionAlready = false;
	float xProjected = 0.F;
	float yProjected = 0.F;
	int inX = 0;  // direction in which the filled region is from this corner. May only be 1 or -1
	int inY = 0;
	PathElement(int x, int y, int inX, int inY);
	PathElement(float xProjected, float yProjected, int x, int y, int inX, int inY);
};

struct HatchesCallParams {
	
	struct HatchPoints {
		int start;
		int count;
	};
	std::vector<HatchPoints> points;
	
	int originX = 0;
	int originY = 0;
};

class DrawOutlineCallParams {
public:
	D3DCOLOR outlineColor{ 0 };
	int thickness = 0;
	
	bool hatched = false;
	HatchesCallParams hatches;
	
	// You must reserve size first for N elems and only then add them
	void reserveSize(int numPathElems);
	void addPathElem(int x, int y, int inX, int inY);
	void addPathElem(float xProjected, float yProjected, int x, int y, int inX, int inY);
	PathElement& getPathElem(int index) const;
	int count() const;
	bool empty() const;
	int getStartPosition() const;
	static PathElement& getPathElemStatic(int startIndex, int index);
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
