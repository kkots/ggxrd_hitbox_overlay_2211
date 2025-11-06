#pragma once
#include "EditedHitbox.h"
#include "Entity.h"
#include "Hitbox.h"
#include "SortedSprite.h"

struct LayerIterator : public EditedHitbox {
	EditedHitbox* layerPtr = nullptr;  // invalid for non-layers
	SortedSprite* parent;
	LayerIterator(Entity ent, SortedSprite* parent);
	LayerIterator(Entity ent);
	LayerIterator(FPAC* fpac, SortedSprite* parent);
	LayerIterator(BYTE* jonbin);
	bool getNext();
	bool getPrev();
	inline void scrollToEnd() { scrolledToEnd = true; reset(); }  // must still call getPrev to get last element
	inline void scrollToStart() { scrolledToEnd = false; reset(); }  // must still call getNext to get first element
	void reset();
	int count();
	int index = 0;
	void copyTo(EditedHitbox* dest) const;
private:
	bool scrolledToEnd = false;
	bool firstRun = true;
	BYTE numTypes;
	short* hitboxCounts = nullptr;
	Hitbox* hitboxesPtr;
	bool eof = false;
	bool sof = false;
	int prevDirection = 0;
	void init();
	BYTE* jonbin;
};
