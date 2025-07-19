#include "pch.h"
#include "DrawOutlineCallParams.h"
#include "logging.h"

DrawOutlineCallParamsManager drawOutlineCallParamsManager;

void DrawOutlineCallParamsManager::onEndSceneStart() {
	allPathElems.clear();
}

PathElement::PathElement(int x, int y, int inX, int inY)
	                    : x(x), y(y), inX(inX), inY(inY) { }

PathElement::PathElement(float xProjected, float yProjected, int x, int y, int inX, int inY)
                        : hasProjectionAlready(true), xProjected(xProjected), yProjected(yProjected), x(x), y(y), inX(inX), inY(inY) { }

int DrawOutlineCallParams::getStartPosition() const {
	return outlineStartAddr;
}

void DrawOutlineCallParams::reserveSize(int numPathElems) {
	outlineStartAddr = drawOutlineCallParamsManager.allPathElems.size();
	internalOutlineAddr = outlineStartAddr;
	outlineCount = numPathElems;
	drawOutlineCallParamsManager.allPathElems.reserve(outlineStartAddr + outlineCount);
}

void DrawOutlineCallParams::addPathElem(int x, int y, int inX, int inY) {
	if (internalOutlineAddr - outlineStartAddr >= outlineCount) {
		logwrap(fprintf(logfile, "Error: putting too many path elements into an outline call params: %d\n", outlineCount));
		return;
	}
	++internalOutlineAddr;
	drawOutlineCallParamsManager.allPathElems.emplace_back(x, y, inX, inY);
}

PathElement& DrawOutlineCallParams::getPathElem(int index) const {
	return drawOutlineCallParamsManager.allPathElems[outlineStartAddr + index];
}

PathElement& DrawOutlineCallParams::getPathElemStatic(int startIndex, int index) {
	return drawOutlineCallParamsManager.allPathElems[startIndex + index];
}

void DrawOutlineCallParams::addPathElem(float xProjected, float yProjected, int x, int y, int inX, int inY) {
	if (internalOutlineAddr - outlineStartAddr >= outlineCount) {
		logwrap(fprintf(logfile, "Error: putting too many path elements into an outline call params: %d\n", outlineCount));
		return;
	}
	++internalOutlineAddr;
	drawOutlineCallParamsManager.allPathElems.emplace_back(xProjected, yProjected, x, y, inX, inY);
}
