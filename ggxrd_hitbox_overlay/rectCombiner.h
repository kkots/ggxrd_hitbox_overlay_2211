#ifndef RECT_COMBINER_H
#define RECT_COMBINER_H

/* This file MIT license  --- Author */

#include <vector>
#include <cstring>

namespace RectCombiner {
	
	class Polygon;

	enum Direction {
		Right,
		DownRight,
		Down,
		DownLeft,
		Left,
		UpLeft,
		Up,
		UpRight
	};

	class PathElement {
	public:
		int x = 0;
		int y = 0;
		Direction dir = UpRight;  // the direction towards which the filled area is
		char xDir() const;  // the direction towards which the filled area is
		char yDir() const;  // the direction towards which the filled area is
	};

	class GridLine {
	public:
		enum ParentType {
			ParentLeft,
			ParentRight,
			ParentTop,
			ParentBottom
		};
		int value = 0;
		int index = 0;
		GridLine* point[2] { nullptr, nullptr };
		Polygon* parent = nullptr;
		ParentType parentType = ParentLeft;
	};

	class Polygon {
	public:
		int left = 0; // these are the input arguments
		int top = 0;
		int right = 0;
		int bottom = 0;
		GridLine* leftLine = nullptr; // these are filled/used during the algorithm
		GridLine* topLine = nullptr;
		GridLine* rightLine = nullptr;
		GridLine* bottomLine = nullptr;
		Polygon() = default;
		Polygon(const Polygon& src) = default;
		Polygon(Polygon&& src) noexcept = default;
		Polygon(int x1, int x2, int y1, int y2);
		Polygon(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4);
		void sortCoordinates();// sorts left, right, top, bottom so that left < right, top < bottom
	};

	// Each GridCell represents three things simultaneously:
	//  The top-left corner of GridCell represents points while
	//  the GridCell's area represents a cell,
	//  and the GridCell's top and left sides represent sides
	//  or gridlines.
	class GridCell {
	public:
		bool filled = false;
		bool topIncluded = false;  // is the top line already included in any outline
		bool leftIncluded = false;  // is the left line already included in any outline
	};

	class GridSpace {
	public:
		std::vector<GridLine> is;
		std::vector<GridLine> js;
		GridCell* cells = nullptr;
		size_t cellCount = 0;  // the number of cells currently allocated in the cells pointer

		// A cell is obtained by the indices of its top left corner.
		// The indices mean the gridlines indices from is, js vectors.
		inline GridCell& getCell(int i, int j);
	};
	
	// This is the only function you need to call to get the results.
	// All outlines are clockwise, even inner ones. To compensate for this, for each PathElement
	// the direction towards which the filled area is is returned.
	// Zero-length lines may be included in the outlines.
	void getOutlines(std::vector<Polygon>& boxes,
	                 std::vector<std::vector<RectCombiner::PathElement>>& outlines,
					 std::vector<Polygon>* newBoxes = nullptr);
	
	// This is for internal use
	void finalizePath(std::vector<GridLine>::iterator& it, GridLine*& i, bool& isFirst, std::vector<std::vector<PathElement>>& outlines);
	
	extern GridSpace gridSpace;
	extern unsigned int sideCount;
	extern std::vector<PathElement> outline;

}

#endif // RECT_COMBINER_H
