
/* This file MIT license  --- Author */

#include "pch.h"
#include "rectCombiner.h"
#include <algorithm>

using namespace RectCombiner;

GridSpace RectCombiner::gridSpace;
unsigned int RectCombiner::sideCount = 0;
std::vector<PathElement> RectCombiner::outline;

inline GridCell& GridSpace::getCell(int i, int j) {
    return cells[(j + 1) * sideCount + (i + 1)];
}

Polygon::Polygon(int x1, int x2, int y1, int y2) {
    if (x1 < x2) {
        left = x1;
        right = x2;
    } else {
        left = x2;
        right = x1;
    }
    if (y1 < y2) {
        top = y1;
        bottom = y2;
    } else {
        top = y2;
        bottom = y1;
    }
}

Polygon::Polygon(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4) {
	
	float minX = x1;
	float maxX = x1;
	float minY = y1;
	float maxY = y1;

	if (x2 < minX) minX = x2;
	if (x3 < minX) minX = x3;
	if (x4 < minX) minX = x4;
	
	if (x2 > maxX) maxX = x2;
	if (x3 > maxX) maxX = x3;
	if (x4 > maxX) maxX = x4;

	if (y2 < minY) minY = y2;
	if (y3 < minY) minY = y3;
	if (y4 < minY) minY = y4;

	if (y2 > maxY) maxY = y2;
	if (y3 > maxY) maxY = y3;
	if (y4 > maxY) maxY = y4;

	left = (int)minX;
	right = (int)maxX;
	top = (int)minY;
	bottom = (int)maxY;
}

void Polygon::sortCoordinates() {
    int temp;
    if (left > right) {
        temp = left;
        left = right;
        right = temp;
    }

    if (top > bottom) {
        temp = top;
        top = bottom;
        bottom = temp;
    }
}

char PathElement::xDir() const {
    switch (dir) {
    case DownRight: return 1;
    case DownLeft: return -1;
    case UpLeft: return -1;
    case UpRight: return 1;
    default: return 1;
    }
}

char PathElement::yDir() const {
    switch (dir) {
    case DownRight: return 1;
    case DownLeft: return 1;
    case UpLeft: return -1;
    case UpRight: return -1;
    default: return 1;
    }
}

void RectCombiner::finalizePath(std::vector<GridLine>::iterator& it, GridLine*& i, bool& isFirst, std::vector<std::vector<PathElement>>& outlines) {
	if (!isFirst) {
		outlines.push_back(outline);
		isFirst = true;
		outline.clear();
	}

	i = &(*it);
}

void RectCombiner::getOutlines(std::vector<Polygon>& boxes,
                               std::vector<std::vector<RectCombiner::PathElement>>& outlines,
                               std::vector<Polygon>* newBoxes) {
	outlines.clear();
    if (boxes.empty()) {
        return;
    }
	if (boxes.size() == 1) {
		Polygon& box = boxes.front();
		outlines.resize(1);
		std::vector<PathElement>& resultOutline = outlines.front();
		resultOutline.reserve(4);
		resultOutline.push_back({box.left, box.top, DownRight});
		resultOutline.push_back({box.right, box.top, DownLeft});
		resultOutline.push_back({box.right, box.bottom, UpLeft});
		resultOutline.push_back({box.left, box.bottom, UpRight});
		return;
	}
    const unsigned int boxesCount = boxes.size();
    sideCount = 2 * boxesCount;
    gridSpace.is.resize(sideCount);
    gridSpace.js.resize(sideCount);
    int count = 0;
    for (Polygon& box : boxes) {
        box.sortCoordinates();
        GridLine* leftLine;
        GridLine* rightLine;
        GridLine* topLine;
        GridLine* bottomLine;

        leftLine = &gridSpace.is[count];
        leftLine->value = box.left;
        leftLine->parent = &box;
        leftLine->parentType = GridLine::ParentType::ParentLeft;

        rightLine = &gridSpace.is[count + 1];
        rightLine->value = box.right;
        rightLine->parent = &box;
        rightLine->parentType = GridLine::ParentType::ParentRight;

        topLine = &gridSpace.js[count];
        topLine->value = box.top;
        topLine->parent = &box;
        topLine->parentType = GridLine::ParentType::ParentTop;

        bottomLine = &gridSpace.js[count + 1];
        bottomLine->value = box.bottom;
        bottomLine->parent = &box;
        bottomLine->parentType = GridLine::ParentType::ParentBottom;

        count += 2;

    }

    std::sort(gridSpace.js.begin(), gridSpace.js.end(), [](const GridLine& a, const GridLine& b) {
        return a.value < b.value;
    });

    count = 0;
    for (auto it = gridSpace.js.begin(); it != gridSpace.js.end(); ++it) {
        GridLine& gridline = *it;
        gridline.index = count;
        switch (gridline.parentType) {
        case GridLine::ParentType::ParentTop: gridline.parent->topLine = &gridline; break;
        case GridLine::ParentType::ParentBottom: gridline.parent->bottomLine = &gridline; break;
        }
        ++count;
    }

    std::sort(gridSpace.is.begin(), gridSpace.is.end(), [](const GridLine& a, const GridLine& b) {
        return a.value < b.value;
    });

    count = 0;
    for (auto it = gridSpace.is.begin(); it != gridSpace.is.end(); ++it) {
        GridLine& gridline = *it;
        gridline.index = count;
        switch (gridline.parentType) {
        case GridLine::ParentType::ParentLeft: gridline.parent->leftLine = &gridline; break;
        case GridLine::ParentType::ParentRight: gridline.parent->rightLine = &gridline; break;
        }
        ++count;
    }

    ++sideCount;
    unsigned int cellsCount = sideCount * sideCount;
    bool needToZeroMemory = true;
    if (gridSpace.cellCount < cellsCount) {
        if (!gridSpace.cells) {
            gridSpace.cells = (GridCell*)calloc(cellsCount, sizeof(GridCell));
            gridSpace.cellCount = cellsCount;
            needToZeroMemory = false;
        } else {
            while (gridSpace.cellCount < cellsCount) {
                gridSpace.cellCount <<= 1;
            }
            gridSpace.cells = (GridCell*)realloc(gridSpace.cells, gridSpace.cellCount * sizeof(GridCell));
        }
    }
    if (!gridSpace.cells) return;
    if (needToZeroMemory) {
        memset(gridSpace.cells, 0, cellsCount * sizeof(GridCell));
    }

    for (const auto& box : boxes) {

        GridLine* currentI = box.leftLine;
        GridLine* currentJ = box.topLine;
        GridCell* cellBase = &gridSpace.getCell(currentI->index, currentJ->index);
        for (; currentI != box.rightLine; ++currentI) {
            currentJ = box.topLine;
            GridCell* cell = cellBase;
            while (currentJ != box.bottomLine) {
                cell->filled = true;
                cell += sideCount;
                ++currentJ;
            }
            ++cellBase;
        }
    }

    if (newBoxes) {
        GridCell* c = gridSpace.cells;
        for (unsigned int j = 0; j < sideCount; ++j) {
            for (unsigned int i = 0; i < sideCount; ++i) {
                if (!c->filled) { ++c; continue; }
                int xLeft, xRight, yTop, yBottom;
                if (i == 0) {
                    xLeft = gridSpace.is[0].value - 20;
                    xRight = xLeft + 20;
                } else if (i == sideCount - 1) {
                    xLeft = gridSpace.is.back().value;
                    xRight = xLeft + 20;
                } else {
                    xLeft = gridSpace.is[i - 1].value;
                    xRight = gridSpace.is[i].value;
                }
                if (j == 0) {
                    yTop = gridSpace.js[0].value - 20;
                    yBottom = yTop + 20;
                } else if (j == sideCount - 1) {
                    yTop = gridSpace.js.back().value;
                    yBottom = yTop + 20;
                } else {
                    yTop = gridSpace.js[j - 1].value;
                    yBottom = gridSpace.js[j].value;
                }
                Polygon resultPolygon{xLeft, xRight, yTop, yBottom};
                resultPolygon.sortCoordinates();
                newBoxes->push_back(resultPolygon);
                ++c;
            }
        }
    }
	
    outline.clear();
    bool isFirst = true;
    Direction lastDir;
    int lastX;
    int lastY;

    for (auto it = gridSpace.is.begin(); it != gridSpace.is.end(); ++it) {
        GridLine* i = &(*it);  // the current point
        for (auto jt = gridSpace.js.begin(); jt != gridSpace.js.end(); ++jt) {
            GridLine* j = &(*jt);
            // these are cells (areas) by direction to which they are from the current point
            GridCell* br = &gridSpace.getCell(i->index, j->index);  // bottom right
            GridCell* tr = br - sideCount;  // top right
            GridCell* bl = br - 1;  // bottom left

            // if point lies on a side that's already included in some kind of outline - skip
            if (tr->leftIncluded || br->leftIncluded || br->topIncluded || bl->topIncluded) continue;

            GridCell* tl = br - sideCount - 1;  // cell which is to the top left from the current point
            GridCell* startPoint = br;
            GridCell* prevPoint = br;

            do {
                br = &gridSpace.getCell(i->index, j->index);  // bottom right
                tr = br - sideCount;  // top right
                bl = br - 1;  // bottom left
                tl = br - sideCount - 1;  // cell which is to the top left from the current point
                GridCell* const rightPoint = br + 1;
                GridCell* const bottomPoint = br + sideCount;

                // four directions:
                // |   side         | direction | point           | next i  | next j |
                // |tr->leftIncluded|  up       |  tr             | i       | j->prev|
                // |br->leftIncluded|  down     |  bottomPoint    | i       | j->next|
                // |br->topIncluded |  right    |  rightPoint     | i->next | j      |
                // |bl->topIncluded |  left     |  bl             | i->prev | j      |

                if (tl->filled) {
                    // +----+
                    // |    |
                    // +----X
                    //

                    if (!bl->filled && bl != prevPoint) {
                        // +----+
                        // |    |
                        // +----X
                        //  <----
                        //
                        bl->topIncluded = true;
                        if (lastDir == Down) {
                            outline.push_back({lastX, lastY, UpLeft});
                            lastDir = Left;
                        } else if (lastDir == Up) {
                            outline.push_back({lastX, lastY, UpRight});
                            lastDir = Left;
                        }
                        --i;
                        lastX = i->value;
                        prevPoint = br;
                        continue;
                    } else if (!tr->filled && tr != prevPoint) {
                        // +----+ ^
                        // |    | ^
                        // +----X ^
                        //
                        tr->leftIncluded = true;
                        if (lastDir == Right) {
                            outline.push_back({lastX, lastY, UpLeft});
                            lastDir = Up;
                        } else if (lastDir == Left) {
                            outline.push_back({lastX, lastY, DownLeft});
                            lastDir = Up;
                        }
                        if (tr == startPoint) {
                            finalizePath(it, i, isFirst, outlines);
                            break;
                        }
                        --j;
                        lastY = j->value;
                        prevPoint = br;
                        continue;
                    }
                }
                if (tr->filled) {
                    //   +----+
                    //   |    |
                    //   X----+
                    //
                    if (!tl->filled && tr != prevPoint) {
                        // ^ +----+
                        // ^ |    |
                        // ^ X----+
                        //
                        tr->leftIncluded = true;
                        if (lastDir == Left) {
                            outline.push_back({lastX, lastY, UpRight});
                            lastDir = Up;
                        } else if (lastDir == Right) {
                            outline.push_back({lastX, lastY, DownRight});
                            lastDir = Up;
                        }
                        if (tr == startPoint) {
                            finalizePath(it, i, isFirst, outlines);
                            break;
                        }
                        --j;
                        lastY = j->value;
                        prevPoint = br;
                        continue;
                    } else if (!br->filled && rightPoint != prevPoint) {
                        // This is the only code that can start an inner contour
                        // and it will always start at its top left corner.
                        //   +----+
                        //   |    |
                        //   X----+
                        //    --->
                        //
                        if (isFirst) {
                            lastY = j->value;
                            outline.push_back({i->value, lastY, UpLeft});
                            isFirst = false;
                            lastDir = Right;
                        }
                        br->topIncluded = true;
                        if (lastDir == Down) {
                            outline.push_back({lastX, lastY, UpRight});
                            lastDir = Right;
                        } else if (lastDir == Up) {
                            outline.push_back({lastX, lastY, UpLeft});
                            lastDir = Right;
                        }
                        ++i;
                        lastX = i->value;
                        prevPoint = br;
                        continue;
                    }
                }
                if (br->filled) {
                    //
                    //   X----+
                    //   |    |
                    //   +----+
                    //
                    if (!tr->filled && rightPoint != prevPoint) {
                        // This is the only code that can start an outer contour
                        // and it will always start at its top left corner.
                        //   ---->
                        //   X----+
                        //   |    |
                        //   +----+
                        //
                        if (isFirst) {
                            lastY = j->value;
                            outline.push_back({i->value, lastY, DownRight});
                            isFirst = false;
                            lastDir = Right;
                        }
                        br->topIncluded = true;
                        if (lastDir == Up) {
                            outline.push_back({lastX, lastY, DownRight});
                            lastDir = Right;
                        } else if (lastDir == Down) {
                            outline.push_back({lastX, lastY, DownLeft});
                            lastDir = Right;
                        }
                        ++i;
                        lastX = i->value;
                        prevPoint = br;
                        continue;
                    } else if (!bl->filled && bottomPoint != prevPoint) {
                        //
                        // v X----+
                        // v |    |
                        // v +----+
                        //
                        br->leftIncluded = true;
                        if (lastDir == Left) {
                            outline.push_back({lastX, lastY, DownRight});
                            lastDir = Down;
                        } else if (lastDir == Right) {
                            outline.push_back({lastX, lastY, UpRight});
                            lastDir = Down;
                        }
                        if (bottomPoint == startPoint) {
                            finalizePath(it, i, isFirst, outlines);
                            break;
                        }
                        ++j;
                        lastY = j->value;
                        prevPoint = br;
                        continue;
                    }
                }
                if (bl->filled) {
                    //
                    //   +----X
                    //   |    |
                    //   +----+
                    //
                    if (!br->filled && bottomPoint != prevPoint) {
                        //
                        //   +----X v
                        //   |    | v
                        //   +----+ v
                        //
                        br->leftIncluded = true;
                        if (lastDir == Right) {
                            outline.push_back({lastX, lastY, DownLeft});
                            lastDir = Down;
                        } else if (lastDir == Left) {
                            outline.push_back({lastX, lastY, UpLeft});
                            lastDir = Down;
                        }
                        if (bottomPoint == startPoint) {
                            finalizePath(it, i, isFirst, outlines);
                            break;
                        }
                        ++j;
                        lastY = j->value;
                        prevPoint = br;
                        continue;
                    } else if (!tl->filled && bl != prevPoint) {
                        //   <---
                        //   +----X
                        //   |    |
                        //   +----+
                        //
                        bl->topIncluded = true;
                        if (lastDir == Up) {
                            outline.push_back({lastX, lastY, DownLeft});
                            lastDir = Left;
                        } else if (lastDir == Down) {
                            outline.push_back({lastX, lastY, DownRight});
                            lastDir = Left;
                        }
                        --i;
                        lastX = i->value;
                        prevPoint = br;
                        continue;
                    }
                }
                break;
            } while (true);
        }
    }
	
    gridSpace.is.clear();
    gridSpace.js.clear();
    return;
}
